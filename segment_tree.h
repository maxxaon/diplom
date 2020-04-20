#pragma once

#include <limits>
#include <memory>

struct IndexSegment {
    size_t left;
    size_t right;

    size_t length() const;

    bool empty() const;

    bool Contains(IndexSegment other) const;
};

IndexSegment IntersectSegments(IndexSegment lhs, IndexSegment rhs);

bool AreSegmentsIntersected(IndexSegment lhs, IndexSegment rhs);

struct IndexWithValue {
    int value = std::numeric_limits<int>::min();
    size_t index = -1;
};
bool operator < (const IndexWithValue& lhs, const IndexWithValue& rhs);

struct BulkAdder {
    int delta = 0;
};

class BulkLinearUpdater {
public:
    BulkLinearUpdater();

    BulkLinearUpdater(const BulkAdder& adder);

    void CombineWith(const BulkLinearUpdater& other);

    IndexWithValue Collapse(IndexWithValue origin, IndexSegment segment) const;

private:
    BulkAdder adder_;
};


template <typename Data, typename BulkOperation>
class MaxSegmentTree {
public:
    explicit MaxSegmentTree(size_t size) : root_(Build({0, size})) {}

    Data ComputeMax(IndexSegment segment) const {
        segment.right += 1;
        return this->TraverseWithQuery(root_, segment, ComputeMaxVisitor{});
    }

    void AssignMinValue(size_t index) {
        return this->TraverseWithQuery(root_, {index, index + 1}, AssignMinValueOperationVisitor{});
    }

    void BulkAdd(IndexSegment segment, int delta) {
        segment.right += 1;
        this->TraverseWithQuery(root_, segment, AddBulkOperationVisitor{BulkAdder{delta}});
    }

private:
    struct Node;
    using NodeHolder = std::unique_ptr<Node>;

    struct Node {
        NodeHolder left;
        NodeHolder right;
        IndexSegment segment;
        Data data;
        BulkOperation postponed_bulk_operation;
    };

    NodeHolder root_;

    static NodeHolder Build(IndexSegment segment) {
        if (segment.empty()) {
            return nullptr;
        } else if (segment.length() == 1) {
            return std::make_unique<Node>(Node{
                    .left = nullptr,
                    .right = nullptr,
                    .segment = segment,
                    .data = {0, segment.left}
            });
        } else {
            const size_t middle = segment.left + segment.length() / 2;
            auto res = std::make_unique<Node>(Node{
                    .left = Build({segment.left, middle}),
                    .right = Build({middle, segment.right}),
                    .segment = segment
            });
            res->data = std::max(
                    (res->left ? res->left->data : Data()),
                    (res->right ? res->right->data : Data()));
            return res;
        }
    }

    template <typename Visitor>
    static typename Visitor::ResultType TraverseWithQuery(const NodeHolder& node, IndexSegment query_segment, Visitor visitor) {
        if (!node || !AreSegmentsIntersected(node->segment, query_segment)) {
            return visitor.ProcessEmpty(node);
        } else {
            PropagateBulkOperation(node);
            if (query_segment.Contains(node->segment)) {
                return visitor.ProcessFull(node);
            } else {
                if constexpr (std::is_void_v<typename Visitor::ResultType>) {
                    TraverseWithQuery(node->left, query_segment, visitor);
                    TraverseWithQuery(node->right, query_segment, visitor);
                    return visitor.ProcessPartial(node, query_segment);
                } else {
                    return visitor.ProcessPartial(
                            node, query_segment,
                            TraverseWithQuery(node->left, query_segment, visitor),
                            TraverseWithQuery(node->right, query_segment, visitor)
                    );
                }
            }
        }
    }

    class ComputeMaxVisitor {
    public:
        using ResultType = Data;

        Data ProcessEmpty(const NodeHolder&) const {
            return {};
        }

        Data ProcessFull(const NodeHolder& node) const {
            return node->data;
        }

        Data ProcessPartial(const NodeHolder&, IndexSegment, const Data& left_result, const Data& right_result) const {
            return std::max(left_result, right_result);
        }
    };

    class AssignMinValueOperationVisitor {
    public:
        using ResultType = void;

        void ProcessEmpty(const NodeHolder&) const {}

        void ProcessFull(const NodeHolder& node) const {
            node->data = Data();
        }

        void ProcessPartial(const NodeHolder& node, IndexSegment) const {
            node->data = std::max(
                    (node->left ? node->left->data : Data()),
                    (node->right ? node->right->data : Data()));
        }
    };

    class AddBulkOperationVisitor {
    public:
        using ResultType = void;

        explicit AddBulkOperationVisitor(const BulkOperation& operation)
                : operation_(operation)
        {}

        void ProcessEmpty(const NodeHolder&) const {}

        void ProcessFull(const NodeHolder& node) const {
            node->postponed_bulk_operation.CombineWith(operation_);
            node->data = operation_.Collapse(node->data, node->segment);
        }

        void ProcessPartial(const NodeHolder& node, IndexSegment) const {
            node->data = std::max(
                    (node->left ? node->left->data : Data()),
                    (node->right ? node->right->data : Data()));
        }

    private:
        const BulkOperation& operation_;
    };

    static void PropagateBulkOperation(const NodeHolder& node) {
        for (auto* child_ptr : {node->left.get(), node->right.get()}) {
            if (child_ptr) {
                child_ptr->postponed_bulk_operation.CombineWith(node->postponed_bulk_operation);
                child_ptr->data = node->postponed_bulk_operation.Collapse(child_ptr->data, child_ptr->segment);
            }
        }
        node->postponed_bulk_operation = BulkOperation();
    }
};

class MaxSegmentTreeOnValueIndex : public MaxSegmentTree<IndexWithValue, BulkLinearUpdater> {
public:
    using MaxSegmentTree::MaxSegmentTree;
};


