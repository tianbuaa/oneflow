/*
Copyright 2020 The OneFlow Authors. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#ifndef ONEFLOW_CORE_GRAPH_STAGE_CHAIN_GRAPH_H_
#define ONEFLOW_CORE_GRAPH_STAGE_CHAIN_GRAPH_H_

#include <utility>
#include <memory>
#include "oneflow/core/graph/graph.h"
#include "oneflow/core/common/container_util.h"

namespace oneflow {

class StageChainEdge;
class StageChainGraph;
class ComputeNode;
class ComputeGraph;

class StageChainNode : public Node<StageChainNode, StageChainEdge> {
 public:
  StageChainNode(const StageChainNode&) = delete;
  StageChainNode(StageChainNode&&) = delete;
  ~StageChainNode() = default;

  template<typename... Args>
  static Maybe<StageChainNode*> UnsafeNew(Args&&... args) {
    auto* stage_op_node = new StageChainNode();
    JUST(stage_op_node->Init(std::forward<Args>(args)...));
    return stage_op_node;
  }

  int64_t stage_placement_id() const { return stage_placement_id_; }
  int64_t parallel_desc_symbol_id() const { return parallel_desc_symbol_id_; }
  int64_t stage_buffer_size() const { return stage_buffer_size_; }
  const std::string& calculation_pass_name() const { return calculation_pass_name_; }

  const HashSet<const ComputeNode*>& compute_nodes() const { return *compute_nodes_; }

  Maybe<void> ForEachBackboneSourceComputeNode(
      const std::function<Maybe<void>(const ComputeNode&)>& DoEach) const;

  Maybe<void> ForEachSourceComputeNode(
      const std::function<Maybe<void>(const ComputeNode&)>& DoEach) const;

  Maybe<void> ForEachSinkComputeNode(
      const std::function<Maybe<void>(const ComputeNode&)>& DoEach) const;

  std::string VisualStr() const override;

 private:
  StageChainNode() = default;
  Maybe<void> Init(int64_t stage_placement_id, int64_t parallel_desc_symbol_id,
                   int64_t stage_buffer_size, const std::string& calculation_pass_name,
                   const std::shared_ptr<HashSet<const ComputeNode*>>& compute_nodes) {
    stage_placement_id_ = stage_placement_id;
    parallel_desc_symbol_id_ = parallel_desc_symbol_id;
    stage_buffer_size_ = stage_buffer_size;
    calculation_pass_name_ = calculation_pass_name;
    compute_nodes_ = compute_nodes;
    return Maybe<void>::Ok();
  }

  int64_t stage_placement_id_;
  int64_t parallel_desc_symbol_id_;
  int64_t stage_buffer_size_;
  std::string calculation_pass_name_;
  std::shared_ptr<HashSet<const ComputeNode*>> compute_nodes_;
};

class StageChainEdge : public Edge<StageChainNode, StageChainEdge> {
 public:
  StageChainEdge(const StageChainEdge&) = delete;
  StageChainEdge(StageChainEdge&&) = delete;
  StageChainEdge() = default;
  ~StageChainEdge() = default;

  const HashSet<std::string>& op_names() const { return op_names_; }
  const HashSet<LogicalBlobId>& lbis() const { return lbis_; }

  Maybe<size_t> NumStagePlacementInPath() const;
  Maybe<size_t> NumParallelDescInPath() const;

  void add_lbi(const LogicalBlobId& lbi);

  void add_path_stage_placement_id(int64_t stage_placement_id);
  void add_path_parallel_desc_symbol_id(int64_t parallel_desc_symbol_id);

  std::string VisualStr() const override;

 private:
  HashSet<LogicalBlobId> lbis_;
  HashSet<std::string> op_names_;
  std::unique_ptr<HashSet<int64_t>> path_stage_placement_ids_;
  std::unique_ptr<HashSet<int64_t>> path_parallel_desc_symbol_ids_;
};

class StageChainGraph : public Graph<StageChainNode, StageChainEdge> {
 public:
  StageChainGraph(const StageChainGraph&) = delete;
  StageChainGraph(StageChainGraph&&) = delete;
  StageChainGraph() = default;
  ~StageChainGraph() = default;

  template<typename... Args>
  static Maybe<StageChainGraph> New(Args&&... args) {
    auto graph = std::make_shared<StageChainGraph>();
    JUST(graph->Init(std::forward<Args>(args)...));
    return graph;
  }

  Maybe<const StageChainNode&> StageChainNode4OpName(const std::string& op_name) const;

  Maybe<void> InitEdgeStatistics();

  Maybe<void> WithDotEndGetter(const std::function<Maybe<std::string>()>& get_dot_end,
                               const std::function<Maybe<void>()>& Do);

  Maybe<std::string> VirtualDotEnd() const override;

 private:
  Maybe<StageChainNode*> MutStageChainNode4OpName(const std::string& op_name);
  Maybe<void> Init(const ComputeGraph& compute_graph);
  Maybe<void> InitNodes(const ComputeGraph& compute_graph);
  Maybe<void> InitEdges(const ComputeGraph& compute_graph);

  void MakeGetterFindOrCreateEdge(
      std::function<StageChainEdge*(StageChainNode* src, StageChainNode* dst)>* FindOrCreateEdge);

  Maybe<void> MakeGetterOtherStageAncestors4ComputeNode(
      const ComputeGraph& compute_graph,
      std::function<Maybe<const std::set<const ComputeNode*>&>(const ComputeNode&)>*
          OtherStageAncestors4ComputeNode) const;

  HashMap<std::string, StageChainNode*> op_name2chain_node_;
  std::function<Maybe<std::string>()> get_dot_end_;
};

}  // namespace oneflow

#endif  // ONEFLOW_CORE_GRAPH_STAGE_CHAIN_GRAPH_H_