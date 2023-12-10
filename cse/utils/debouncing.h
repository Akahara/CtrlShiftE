#pragma once

#include <mutex>
#include <semaphore>

namespace won
{

template <class T>
struct function_traits;

template <class ClassType, class ReturnType, class... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)>
{
  enum { arity = sizeof...(Args) };

  using class_type = ClassType;
  using result_type = ReturnType;
  using args_type = std::tuple<Args...>;
  using allargs_type = std::tuple<ClassType*, Args...>;
};



struct debouncing {
  using clock = std::chrono::system_clock;

  // concurreny policies decide what to do when a debounced function is being called
  // when the underlying function is running
  struct concurrent_wait {};    // wait until the underlying function finishes and prepare another call
  struct concurrent_discard {}; // do nothing, the arguments passed will be discarded
  struct concurrent_queue {};   // queue a new call without blocking, if this happens multiple time the previous queued call is discarded
};

template<class func, class concurrent_policy=debouncing::concurrent_queue>
class debounced {
public:
  using clock = debouncing::clock;
  using func_traits = function_traits<func>;
  using args_type = typename func_traits::args_type;
  using target_type = typename func_traits::class_type;

public:
  debounced(func f, target_type *target, clock::duration debounceDelay)
    : debounceDelay(debounceDelay)
    , function(f)
    , target(target)
    , started(0)
    , thread([this] { threadLoop(); })
  {
    started.acquire();
    runningMutex.lock();
    runningMutex.unlock();
  }

  debounced(const debounced &) = delete;
  debounced(debounced &&) = delete;
  debounced &operator=(const debounced &) = delete;
  debounced &operator=(debounced &&) = delete;

  ~debounced()
  {
    {
      std::lock_guard _{ runningMutex };
      isDying = true;
    }
    condVar.notify_one();
    thread.join();
  }

  void operator()(auto &&...args)
  {
    auto prepareNext = [&](args_type &argsDest) {
      argsDest = std::make_tuple(std::forward<decltype(args)>(args)...);
      callTime = clock::now() + debounceDelay;
      condVar.notify_one();
    };

    if constexpr (std::is_same_v<concurrent_policy, debouncing::concurrent_wait>) {
      std::lock_guard _{ runningMutex };
      prepareNext(latestArgs);
      areArgsPending = true;
    } else if constexpr (std::is_same_v<concurrent_policy, debouncing::concurrent_discard>) {
      if (!runningMutex.try_lock()) return;
      prepareNext(latestArgs);
      areArgsPending = true;
      runningMutex.unlock();
    } else if constexpr (std::is_same_v<concurrent_policy, debouncing::concurrent_queue>) {
      if (!runningMutex.try_lock()) {
        std::lock_guard _{ queueMutex };
        prepareNext(queuedArgs);
        areArgsQueued = true;
      } else {
        prepareNext(latestArgs);
        areArgsPending = true;
        runningMutex.unlock();
      }
    } else {
      struct invalid_concurrent_policy _;
    }
  }

private:
  void threadLoop()
  {
    std::unique_lock lock{ runningMutex };
    started.release();
    while(!isDying) {
      condVar.wait(lock, [&] { return areArgsPending || isDying; });
      if (isDying) break;
      while (clock::now() < callTime && !isDying) {
        lock.unlock();
        std::this_thread::sleep_for(callTime - clock::now());
        lock.lock();
      }
      if (isDying) break;

      performFunctionCall();
      areArgsPending = false;

      // if a call was queued durring the previous call execution
      // then do not go back to sleep and run the next call immediately
      if constexpr (std::is_same_v<concurrent_policy, debouncing::concurrent_queue>) {
        std::unique_lock queueLock{ queueMutex };
        while(areArgsQueued) {
          std::swap(queuedArgs, latestArgs);
          areArgsQueued = false;
          queueLock.unlock();
          performFunctionCall();
          queueLock.lock();
        }
      }
    }
  }

  void performFunctionCall() {
    // call the debounced function
    [&]<size_t... S>(std::index_sequence<S...>) {
      std::invoke(function, target, std::get<S>(latestArgs)...);
    }(std::make_index_sequence<std::tuple_size_v<args_type>>{});
  }

private:
  // settings
  clock::duration debounceDelay;
  func function;
  target_type *target;

  // runtime arguments
  clock::time_point callTime;
  args_type latestArgs;
  args_type queuedArgs;

  // synchronisation
  std::mutex runningMutex;
  std::mutex queueMutex;
  std::atomic_bool isDying{ false };
  std::atomic_bool areArgsPending{ false };
  bool areArgsQueued = false;
  std::condition_variable condVar;
  std::binary_semaphore started;
  std::thread thread;
};

}
