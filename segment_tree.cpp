#include "segment_tree.h"

size_t IndexSegment::length() const {
    return right - left;
}
bool IndexSegment::empty() const {
    return length() == 0;
}

bool IndexSegment::Contains(IndexSegment other) const {
    return left <= other.left && other.right <= right;
}

IndexSegment IntersectSegments(IndexSegment lhs, IndexSegment rhs) {
    const size_t left = std::max(lhs.left, rhs.left);
    const size_t right = std::min(lhs.right, rhs.right);
    return {left, std::max(left, right)};
}

bool AreSegmentsIntersected(IndexSegment lhs, IndexSegment rhs) {
    return !(lhs.right <= rhs.left || rhs.right <= lhs.left);
}

bool operator < (const IndexWithValue& lhs, const IndexWithValue& rhs) {
    return std::tie(lhs.value, lhs.index) < std::tie(rhs.value, rhs.index);
}

BulkLinearUpdater::BulkLinearUpdater() = default;

BulkLinearUpdater::BulkLinearUpdater(const BulkAdder& adder)
        : adder_(adder)
{}

void BulkLinearUpdater::CombineWith(const BulkLinearUpdater& other) {
    adder_.delta += other.adder_.delta;
}

IndexWithValue BulkLinearUpdater::Collapse(IndexWithValue origin, IndexSegment segment) const {
    return {origin.value + adder_.delta, origin.index};
}



