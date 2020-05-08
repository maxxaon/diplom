#include "segment_tree.h"
#include "simulator.h"

#include <algorithm>
#include <unordered_set>

Channel::Channel(double speed, double delay) : speed_(speed), delay_(delay) {}

double Channel::get_transfer_time(double data_volume) const {
    return delay_ + (data_volume / 1000) / speed_;
}

Simulator::Simulator(size_t total_page_count, std::vector<PageAccess> access_history, Channel channel)
        : total_page_count_(total_page_count), access_history_(move(access_history)), channel_(channel) {

}

Criterias Simulator::RunMigration(MigrationScheme migration_scheme, bool optimization_flag) {
    Clear();
    if (migration_scheme == MigrationScheme::PreCopy) {
        return RunPreCopyMigration(optimization_flag);
    } else if (migration_scheme == MigrationScheme::PostCopy) {
        return RunPostCopyMigration(optimization_flag);
    } else {
        throw std::runtime_error("Unknown migration scheme");
    }
}

Criterias Simulator::RunPreCopyMigration(bool optimization_flag) {
    double transmitted_data = 0;
    int iteration_number = 0;
    std::vector<int> change_number(total_page_count_);
    while (iteration_number < PreCopyStopParameters::max_iteration_count
           && 1.0 * pages_to_transfer_.size() / total_page_count_
              > PreCopyStopParameters::ok_leave_pages_part) {
        // start iteration

        if (optimization_flag) {
            std::sort(pages_to_transfer_.begin(), pages_to_transfer_.end(),
                 [&change_number] (const int lhs, const int rhs) {
                     return change_number[lhs] < change_number[rhs];
                 });
        }

        std::unordered_set<int> next_pages_to_transfer;
        std::unordered_set<int> already_sent_pages;

        while (!pages_to_transfer_.empty()) {
            // transfer next page

            int cur_page_to_transfer = pages_to_transfer_.front();
            pages_to_transfer_.pop_front();

            double data_volume = page_size_;
            transmitted_data += data_volume;
            cur_time_ += channel_.get_transfer_time(data_volume);
            already_sent_pages.insert(cur_page_to_transfer);

            // if we have access to a page after transferring, add it to next iteration
            while (auto accessed_page_number_opt = get_next_accessed_page_number(Operation::Write)) {
                int accessed_page_number = *accessed_page_number_opt;
                if (already_sent_pages.find(accessed_page_number) != already_sent_pages.end()
                        && next_pages_to_transfer.find(accessed_page_number) == next_pages_to_transfer.end()) {
                    next_pages_to_transfer.insert(accessed_page_number);
                }
            }
        }

        pages_to_transfer_ = std::deque<int>(next_pages_to_transfer.begin(), next_pages_to_transfer.end());
    }

    double data_volume = pages_to_transfer_.size() * page_size_;
    pages_to_transfer_.clear();
    transmitted_data += data_volume;
    double downtime = channel_.get_transfer_time(data_volume);
    cur_time_ += downtime;

    return Criterias{
            .downtime = downtime,
            .delays = 0,
            .total_migration_time = cur_time_,
            .transmitted_data = transmitted_data,
            .eviction_time = cur_time_
    };
}

IndexSegment get_index_segment_by_k(int page_number, double k) {
    return {size_t((1 - k) * page_number), size_t((1 + k) * page_number)};
}

Criterias Simulator::RunPostCopyMigration(bool optimization_flag) {
    double transmitted_data = 0;
    int miss_count = 0;
    std::unordered_set<int> already_sent_pages;

    std::optional<MaxSegmentTreeOnValueIndex> segment_tree_opt;
    if (optimization_flag) {
        *segment_tree_opt = MaxSegmentTreeOnValueIndex(total_page_count_);
    }
    while (already_sent_pages.size() != total_page_count_) {
        int cur_page_to_transfer;
        if (optimization_flag) {
            cur_page_to_transfer =
                    segment_tree_opt->ComputeMax({0, total_page_count_ - 1}).index;
            segment_tree_opt->AssignMinValue(cur_page_to_transfer);
        } else {
            cur_page_to_transfer = pages_to_transfer_.front();
            pages_to_transfer_.pop_front();
        }

        double data_volume = page_size_;
        transmitted_data += data_volume;
        cur_time_ += channel_.get_transfer_time(data_volume);
        already_sent_pages.insert(cur_page_to_transfer);

        while (auto accessed_page_number_opt = get_next_accessed_page_number(Operation::Read)) {
            int accessed_page_number = *accessed_page_number_opt;
            if (already_sent_pages.find(accessed_page_number) == already_sent_pages.end()) {
                miss_count += 1;

                delays_ += channel_.get_transfer_time(page_num_size_) + channel_.get_transfer_time(page_size_);
                transmitted_data += page_num_size_ + page_size_;
                cur_time_ += channel_.get_transfer_time(page_size_);
                already_sent_pages.insert(accessed_page_number);

                if (optimization_flag) {
                    segment_tree_opt->BulkAdd(
                            get_index_segment_by_k(accessed_page_number, PostCopyOptimizationParameters::seq_k),
                            miss_count);
                    segment_tree_opt->BulkAdd(
                            get_index_segment_by_k(accessed_page_number, PostCopyOptimizationParameters::local_k),
                            miss_count);
                }
            }
        }
    }

    return Criterias{
            .downtime = 0,
            .delays = delays_,
            .total_migration_time = cur_time_,
            .transmitted_data = transmitted_data,
            .eviction_time = cur_time_,
    };
}

std::optional<int> Simulator::get_next_accessed_page_number(Operation operation) {
    while (access_history_pos_ < access_history_.size()
           && access_history_pos_ * access_gap <= cur_time_) {
        auto cur_access = access_history_[access_history_pos_++];
        if (cur_access.operation == operation) {
            return cur_access.page_number;
        }
    }
    return std::nullopt;
}

void Simulator::Clear() {
    pages_to_transfer_.clear();
    for (int i = 0; i < total_page_count_; ++i) {
        pages_to_transfer_.push_back(i);
    }
    cur_time_ = 0;
    delays_ = 0;
}


