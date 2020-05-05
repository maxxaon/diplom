#pragma once

#include <deque>
#include <vector>
#include <optional>

enum class Operation {
    Read, Write
};

enum class MigrationScheme {
    PreCopy, PostCopy
};

struct PageAccess {
    size_t page_number;
    Operation operation;
};

struct Criterias {
    double downtime = 0;
    double delays = 0;
    double total_migration_time = 0;
    double transmitted_data = 0;
    double eviction_time = 0;
};

class Channel {
public:
    Channel(double speed, double delay);

    double get_transfer_time(double data_volume) const;
private:
    double speed_; // MB/s
    double delay_; // ms
};

class Simulator {
public:
    Simulator(size_t total_page_count, std::vector<PageAccess> access_history, Channel channel);

    Criterias RunMigration(MigrationScheme migration_scheme, bool optimization_flag);

private:
    struct PreCopyStopParameters {
        static const int max_iteration_count = 1000;
        static constexpr double ok_leave_pages_part = 0.01;
    };

    Criterias RunPreCopyMigration(bool optimization_flag);

    struct PostCopyOptimizationParameters {
        static constexpr double seq_k = 0.001;
        static constexpr double local_k = 0.01;
    };

    Criterias RunPostCopyMigration(bool optimization_flag);

    std::optional<int> get_next_accessed_page_number(Operation operation);

    void Clear();

    size_t total_page_count_;
    std::deque<int> pages_to_transfer_;
    std::vector<PageAccess> access_history_;
    size_t access_history_pos_ = 0;
    double cur_time_;
    double delays_;
    Channel channel_;

    const double page_size_ = 4; // KB
    const double page_num_size_ = 0.004; // KB
    const double access_gap = 1e-6; // s
};


