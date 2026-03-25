package bason

import "sort"

// FlattenBason converts a nested BASON record into a sequence of flat path-keyed leaf records.
func FlattenBason(root Record) []Record {
	var result []Record
	flattenRecursive(root, root.Key, &result)
	return result
}

func flattenRecursive(record Record, currentPath string, result *[]Record) {
	if record.Type == TypeArray || record.Type == TypeObject {
		for _, child := range record.Children {
			childPath := currentPath
			if childPath != "" {
				childPath += "/" + child.Key
			} else {
				childPath = child.Key
			}
			flattenRecursive(child, childPath, result)
		}
	} else {
		*result = append(*result, Record{
			Type:  record.Type,
			Key:   currentPath,
			Value: record.Value,
		})
	}
}

type treeNode struct {
	Type     BasonType
	Value    string
	Children map[string]*treeNode
	IsLeaf   bool
}

func buildTree(records []Record) *treeNode {
	root := &treeNode{Type: TypeObject, Children: make(map[string]*treeNode)}
	for _, record := range records {
		segments := SplitPath(record.Key)
		if len(segments) == 0 {
			root.Type = record.Type
			root.Value = record.Value
			root.IsLeaf = true
			continue
		}
		current := root
		for i, segment := range segments {
			isLast := i == len(segments)-1
			node, ok := current.Children[segment]
			if !ok {
				node = &treeNode{Children: make(map[string]*treeNode)}
				current.Children[segment] = node
			}
			if isLast {
				node.Type = record.Type
				node.Value = record.Value
				node.IsLeaf = true
			} else {
				if len(node.Children) == 0 && !node.IsLeaf && i+1 < len(segments) {
					if _, err := DecodeRon64(segments[i+1]); err == nil {
						node.Type = TypeArray
					} else {
						node.Type = TypeObject
					}
				}
				current = node
			}
		}
	}
	return root
}

func treeToRecord(node *treeNode, key string) Record {
	record := Record{Key: key, Type: node.Type}
	if node.IsLeaf {
		record.Value = node.Value
	} else {
		keys := make([]string, 0, len(node.Children))
		for k := range node.Children {
			keys = append(keys, k)
		}
		sort.Strings(keys)
		for _, childKey := range keys {
			record.Children = append(record.Children, treeToRecord(node.Children[childKey], childKey))
		}
	}
	return record
}

// UnflattenBason converts a sequence of flat path-keyed leaf records into a nested BASON record tree.
func UnflattenBason(records []Record) Record {
	if len(records) == 0 {
		return Record{Type: TypeObject}
	}
	tree := buildTree(records)
	return treeToRecord(tree, "")
}
