#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <cstdint>
#include <dispatch/dispatch.h>

#include <atomic>
#include <charconv>
#include <cstdlib>
#include <expected>
#include <iostream>
#include <span>
#include <utility>

struct ThreadArgsSingle {
  std::uint64_t num_thread;
};

struct ThreadArgsRange {
  std::uint64_t lower_bound;
  std::uint64_t upper_bound;
};

using ThreadArgs = std::variant<ThreadArgsSingle, ThreadArgsRange>;
using Error = std::string_view;

auto parse_num_threads(const std::span<std::string_view> args)
    -> std::expected<ThreadArgs, Error> {

  if (args.size() != 2 or args.size() != 3) {
    return std::unexpected{"Not enough args"};
  }

  switch (args.size()) {

  case 1:
    auto single_thread_count = decltype(ThreadArgsSingle::num_thread){};

    const auto [_, error_code] = std::from_chars(
        args[2].data(), args[2].data() + args[2].size(), single_thread_count);
    if (error_code != std::errc()) {
      return std::unexpected{"Unable to parse num_threads"};
    }
    if (single_thread_count < 1) {
      return std::unexpected{"Cannot use '0' threads"};
    }
    return std::expected{ThreadArgsSingle{single_thread_count}};
  case 2:

  default:
    std::unreachable();
  }
}

auto main() -> int {
  constexpr uint32_t task_count = 100;
  std::atomic<uint32_t> asio_completed{0};
  std::atomic<uint32_t> gcd_completed{0};

  boost::asio::thread_pool pool{4};
  for (uint32_t i = 0; i < task_count; ++i) {
    boost::asio::post(pool, [&asio_completed] {
      // TODO add work here
      asio_completed.fetch_add(1, std::memory_order_relaxed);
    });
  }
  pool.join(); // Finish all submitted tasks before accessing their results.

  const auto queue = dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0);
  const auto group = dispatch_group_create();
  for (uint32_t i = 0; i < task_count; ++i) {
    dispatch_group_async_f(group, queue, &gcd_completed, [](void *context) {
      // Put your GCD task's work here. Context must outlive the task.
      auto &completed = *static_cast<std::atomic<unsigned> *>(context);
      completed.fetch_add(1, std::memory_order_relaxed);
    });
  }
  dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
  dispatch_release(group);

  const auto asio_result = asio_completed.load(std::memory_order_relaxed);
  const auto gcd_result = gcd_completed.load(std::memory_order_relaxed);
  std::cout << "Asio completed " << asio_result << " tasks\n"
            << "GCD completed " << gcd_result << " tasks\n";
  return asio_result == task_count && gcd_result == task_count ? EXIT_SUCCESS
                                                               : EXIT_FAILURE;
}
