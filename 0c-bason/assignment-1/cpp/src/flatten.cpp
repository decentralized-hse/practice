#include "flatten.h"
#include "path_utils.h"
#include "ron64.h"

#include <map>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

namespace {

void FlattenRecursive(
    const TBasonRecord& record,
    const std::string& currentPath,
    std::vector<TBasonRecord>& result)
{
    if (record.Type == EBasonType::Array || record.Type == EBasonType::Object) {
        // Container - recurse into children
        for (const auto& child : record.Children) {
            std::string childPath = currentPath.empty() 
                ? child.Key 
                : currentPath + "/" + child.Key;
            FlattenRecursive(child, childPath, result);
        }
    } else {
        // Leaf - add to result
        TBasonRecord flat(record.Type, currentPath, record.Value);
        result.push_back(std::move(flat));
    }
}

struct TTreeNode {
    EBasonType Type = EBasonType::Object;
    std::string Value;
    std::map<std::string, TTreeNode> Children;
    bool IsLeaf = false;
};

void BuildTree(const std::vector<TBasonRecord>& records, TTreeNode& root)
{
    for (const auto& record : records) {
        auto segments = SplitPath(record.Key);
        
        if (segments.empty()) {
            // Root-level leaf
            root.Type = record.Type;
            root.Value = record.Value;
            root.IsLeaf = true;
            continue;
        }

        TTreeNode* current = &root;
        
        for (size_t i = 0; i < segments.size(); ++i) {
            const auto& segment = segments[i];
            bool isLast = (i == segments.size() - 1);
            
            if (isLast) {
                // Leaf node
                auto& node = current->Children[segment];
                node.Type = record.Type;
                node.Value = record.Value;
                node.IsLeaf = true;
            } else {
                // Intermediate node - determine if it's array or object
                auto& node = current->Children[segment];
                if (node.Children.empty() && !node.IsLeaf) {
                    // Try to parse next segment as RON64 to detect arrays
                    if (i + 1 < segments.size()) {
                        try {
                            DecodeRon64(segments[i + 1]);
                            node.Type = EBasonType::Array;
                        } catch (...) {
                            node.Type = EBasonType::Object;
                        }
                    }
                }
                current = &node;
            }
        }
    }
}

void TreeToRecord(const TTreeNode& node, const std::string& key, TBasonRecord& record)
{
    record.Key = key;
    record.Type = node.Type;
    
    if (node.IsLeaf) {
        record.Value = node.Value;
    } else {
        // Container
        for (const auto& [childKey, childNode] : node.Children) {
            TBasonRecord child;
            TreeToRecord(childNode, childKey, child);
            record.Children.push_back(std::move(child));
        }
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

std::vector<TBasonRecord> FlattenBason(const TBasonRecord& root)
{
    std::vector<TBasonRecord> result;
    FlattenRecursive(root, root.Key, result);
    return result;
}

TBasonRecord UnflattenBason(const std::vector<TBasonRecord>& records)
{
    if (records.empty()) {
        return TBasonRecord(EBasonType::Object);
    }

    TTreeNode tree;
    BuildTree(records, tree);

    TBasonRecord result;
    TreeToRecord(tree, "", result);
    
    return result;
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
