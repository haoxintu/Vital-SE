//===-- Searcher.cpp ------------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Searcher.h"

#include "CoreStats.h"
#include "ExecutionState.h"
#include "Executor.h"
#include "MergeHandler.h"
#include "PTree.h"
#include "StatsTracker.h"

#include "klee/ADT/DiscretePDF.h"
#include "klee/ADT/RNG.h"
#include "klee/Statistics/Statistics.h"
#include "klee/Module/InstructionInfoTable.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/ErrorHandling.h"
#include "klee/System/Time.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"
DISABLE_WARNING_POP

#include "llvm/IR/CFG.h" // THX for find the successors
#include "llvm/IR/Dominators.h" // THX
#include "llvm/Analysis/PostDominators.h" // THX
#include "llvm/Pass.h"
#include "llvm/FuzzMutate/RandomIRBuilder.h"
#include <cassert>
#include <cmath>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/Utils/LoopUtils.h>
#include <llvm/Support/raw_ostream.h>

using namespace klee;
using namespace llvm;

// THX
#include <fstream>
#include <sstream>
#include <bits/stdc++.h>
#include <math.h>
#include "UserSearcher.h"
std::set<std::string> globalRecordingSet;
std::map<llvm::BasicBlock*, unsigned int> globalUnsafeBlockMap;
bool is_simulation_mode = 0;
bool enable_simulation = 1;
unsigned int simulation_limit;
bool simulation_opt;
std::set<llvm::Instruction*> globalUnsafeInstructionSet;
//bool exit_simulation_run = 0;
// for debugging purpose only
std::set<PTreeNode* > selectedNodeSet;
std::set<PTreeNode* > expandedNodeSet;
std::map<PTreeNode*, PTreeNode*> expandHistoryMap;
namespace klee {
  extern RNG theRNG;
}
///

ExecutionState &DFSSearcher::selectState() {
  if (debug_thx){
    ;//llvm::errs() << "THX DFS states id: " << (*states.back()).id << "\n";
  }
  return *states.back();
}

void DFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    //llvm::errs() << "DFSSearcher::update state = " << state->isSimulationState() << "\n";
    if (state->isSimulationState())
        continue;
    if (state == states.back()) {
      states.pop_back();
    } else {
      auto it = std::find(states.begin(), states.end(), state);
      assert(it != states.end() && "invalid state removed");
      states.erase(it);
    }
  }
}

bool DFSSearcher::empty() {
  return states.empty();
}

void DFSSearcher::printName(llvm::raw_ostream &os) {
  os << "DFSSearcher\n";
}


///

ExecutionState &BFSSearcher::selectState() {
  if (debug_thx){
    ;//llvm::errs() << "THX BFS states id: " << (*states.front()).id << "\n";
  }
  return *states.front();
}

void BFSSearcher::update(ExecutionState *current,
                         const std::vector<ExecutionState *> &addedStates,
                         const std::vector<ExecutionState *> &removedStates) {
  // update current state
  // Assumption: If new states were added KLEE forked, therefore states evolved.
  // constraints were added to the current state, it evolved.
  if (!addedStates.empty() && current &&
      std::find(removedStates.begin(), removedStates.end(), current) == removedStates.end()) {
    auto pos = std::find(states.begin(), states.end(), current);
    assert(pos != states.end());
    states.erase(pos);
    states.push_back(current);
  }

  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    if (state == states.front()) {
      states.pop_front();
    } else {
      auto it = std::find(states.begin(), states.end(), state);
      assert(it != states.end() && "invalid state removed");
      states.erase(it);
    }
  }
}

bool BFSSearcher::empty() {
  return states.empty();
}

void BFSSearcher::printName(llvm::raw_ostream &os) {
  os << "BFSSearcher\n";
}


///

RandomSearcher::RandomSearcher(RNG &rng) : theRNG{rng} {}

ExecutionState &RandomSearcher::selectState() {
  //if (debug_thx){
  //  llvm::errs() << "THX: randomly select a state\n";
  //}
  return *states[theRNG.getInt32() % states.size()];
}

void RandomSearcher::update(ExecutionState *current,
                            const std::vector<ExecutionState *> &addedStates,
                            const std::vector<ExecutionState *> &removedStates) {
  // insert states
  states.insert(states.end(), addedStates.begin(), addedStates.end());

  // remove states
  for (const auto state : removedStates) {
    auto it = std::find(states.begin(), states.end(), state);
    if (debug_thx) {
        llvm::errs() << "state.isSimulationState() = " << state->isSimulationState() << "\n";
    }
    if (it != states.end() && state->isSimulationState()){ // THX: this should be removed beforehand
        continue;
    }
    assert(it != states.end() && "invalid state removed");
    states.erase(it);
  }
}

bool RandomSearcher::empty() {
  return states.empty();
}

void RandomSearcher::printName(llvm::raw_ostream &os) {
  llvm::errs() << "++++++ This is in RandomSearcher\n";
  os << "RandomSearcher\n";
}


///

WeightedRandomSearcher::WeightedRandomSearcher(WeightType type, RNG &rng)
  : states(std::make_unique<DiscretePDF<ExecutionState*, ExecutionStateIDCompare>>()),
    theRNG{rng},
    type(type) {

  switch(type) {
  case Depth:
  case RP:
    updateWeights = false;
    break;
  case InstCount:
  case CPInstCount:
  case QueryCost:
  case MinDistToUncovered:
  case CoveringNew:
    updateWeights = true;
    break;
  default:
    assert(0 && "invalid weight type");
  }
}

ExecutionState &WeightedRandomSearcher::selectState() {
  return *states->choose(theRNG.getDoubleL());
}

double WeightedRandomSearcher::getWeight(ExecutionState *es) {
  switch(type) {
    default:
    case Depth:
      return es->depth;
    case RP:
      return std::pow(0.5, es->depth);
    case InstCount: {
      uint64_t count = theStatisticManager->getIndexedValue(stats::instructions,
                                                            es->pc->info->id);
      double inv = 1. / std::max((uint64_t) 1, count);
      return inv * inv;
    }
    case CPInstCount: {
      StackFrame &sf = es->stack.back();
      uint64_t count = sf.callPathNode->statistics.getValue(stats::instructions);
      double inv = 1. / std::max((uint64_t) 1, count);
      return inv;
    }
    case QueryCost:
      return (es->queryMetaData.queryCost.toSeconds() < .1)
                 ? 1.
                 : 1. / es->queryMetaData.queryCost.toSeconds();
    case CoveringNew:
    case MinDistToUncovered: {
      uint64_t md2u = computeMinDistToUncovered(es->pc,
                                                es->stack.back().minDistToUncoveredOnReturn);

      double invMD2U = 1. / (md2u ? md2u : 10000);
      if (type == CoveringNew) {
        double invCovNew = 0.;
        if (es->instsSinceCovNew)
          invCovNew = 1. / std::max(1, (int) es->instsSinceCovNew - 1000);
        return (invCovNew * invCovNew + invMD2U * invMD2U);
      } else {
        return invMD2U * invMD2U;
      }
    }
  }
}

void WeightedRandomSearcher::update(ExecutionState *current,
                                    const std::vector<ExecutionState *> &addedStates,
                                    const std::vector<ExecutionState *> &removedStates) {

  // update current
  if (current && updateWeights &&
      std::find(removedStates.begin(), removedStates.end(), current) == removedStates.end())
    states->update(current, getWeight(current));

  // insert states
  for (const auto state : addedStates)
    states->insert(state, getWeight(state));

  // remove states
  for (const auto state : removedStates)
    states->remove(state);
}

bool WeightedRandomSearcher::empty() {
  return states->empty();
}

void WeightedRandomSearcher::printName(llvm::raw_ostream &os) {
  os << "WeightedRandomSearcher::";
  switch(type) {
    case Depth              : os << "Depth\n"; return;
    case RP                 : os << "RandomPath\n"; return;
    case QueryCost          : os << "QueryCost\n"; return;
    case InstCount          : os << "InstCount\n"; return;
    case CPInstCount        : os << "CPInstCount\n"; return;
    case MinDistToUncovered : os << "MinDistToUncovered\n"; return;
    case CoveringNew        : os << "CoveringNew\n"; return;
    default                 : os << "<unknown type>\n"; return;
  }
}


///

// Check if n is a valid pointer and a node belonging to us
#define IS_OUR_NODE_VALID(n)                                                   \
  (((n).getPointer() != nullptr) && (((n).getInt() & idBitMask) != 0))

RandomPathSearcher::RandomPathSearcher(PTree &processTree, RNG &rng)
  : processTree{processTree},
    theRNG{rng},
    idBitMask{processTree.getNextId()} {};

ExecutionState &RandomPathSearcher::selectState() {
  unsigned flips=0, bits=0;
  assert(processTree.root.getInt() & idBitMask && "Root should belong to the searcher");
  PTreeNode *n = processTree.root.getPointer();
  if (debug_thx){
    ;//llvm::errs() << "THX node to explore: " << n << "\n";
  }
  while (!n->state) {
    if (!IS_OUR_NODE_VALID(n->left)) {
      assert(IS_OUR_NODE_VALID(n->right) && "Both left and right nodes invalid");
      assert(n != n->right.getPointer());
      n = n->right.getPointer();
    } else if (!IS_OUR_NODE_VALID(n->right)) {
      assert(IS_OUR_NODE_VALID(n->left) && "Both right and left nodes invalid");
      assert(n != n->left.getPointer());
      n = n->left.getPointer();
    } else {
      if (bits==0) {
        flips = theRNG.getInt32();
        bits = 32;
      }
      --bits;
      n = ((flips & (1U << bits)) ? n->left : n->right).getPointer();
    }
  }
  return *n->state;
}

void RandomPathSearcher::update(ExecutionState *current,
                                const std::vector<ExecutionState *> &addedStates,
                                const std::vector<ExecutionState *> &removedStates) {
  // insert states
  for (auto es : addedStates) {
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;
    PTreeNodePtr *childPtr;

    childPtr = parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                              : &parent->right)
                      : &processTree.root;
    while (pnode && !IS_OUR_NODE_VALID(*childPtr)) {
      childPtr->setInt(childPtr->getInt() | idBitMask);
      pnode = parent;
      if (pnode)
        parent = pnode->parent;

      childPtr = parent
                     ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                             : &parent->right)
                     : &processTree.root;
    }
  }

  // remove states
  for (auto es : removedStates) {
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;

    while (pnode && !IS_OUR_NODE_VALID(pnode->left) &&
           !IS_OUR_NODE_VALID(pnode->right)) {
      auto childPtr =
          parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                         : &parent->right)
                 : &processTree.root;
      assert(IS_OUR_NODE_VALID(*childPtr) && "Removing pTree child not ours");
      childPtr->setInt(childPtr->getInt() & ~idBitMask);
      pnode = parent;
      if (pnode)
        parent = pnode->parent;
    }
  }
}

bool RandomPathSearcher::empty() {
  return !IS_OUR_NODE_VALID(processTree.root);
}

void RandomPathSearcher::printName(llvm::raw_ostream &os) {
  llvm::errs() << "++++++ This is in RandomPathSearcher\n";
  os << "RandomPathSearcher\n";
}


// THX MCTS implementation start
// Check if n is a valid pointer and a node belonging to us
#define IS_OUR_NODE_VALID_MCTS(n)                                                   \
  (((n).getPointer() != nullptr))
  //(((n).getPointer() != nullptr) && (((n).getInt() & idBitMask) != 0))

bool MCTSSearcher::isTerminalNode(PTreeNode* node){
    assert(node != 0);
    bool ret = 0;
    if (node->isTerminal)
        ret = true;
    else
        ret = false;
    return ret;
}

bool MCTSSearcher::hasEligiableChildren(PTreeNode* node){
    // see which one is not in the tree, then check if it is a terminator
    if ((node->left.getPointer() != 0) && (node->right.getPointer() != 0)
                && (!node->left.getPointer()->isInMCTSTree) && (! node->right.getPointer()->isInMCTSTree)){
        if ((! node->left.getPointer()->isTerminal) || (! node->right.getPointer()->isTerminal)){ // has two eligiable nodes
            return true;
        } else
            return false;
    }
    if ((node->left.getPointer() == 0) && (node->right.getPointer() == 0)){
        llvm::errs() << "none of left and right node can be selected\n";
        return false;
    }
    if ((node->left.getPointer() != 0) && ! node->left.getPointer()->isInMCTSTree) {
        if (!node->left.getPointer()->isTerminal)
            return true;
        else
            return false;
    }
    if ((node->right.getPointer() != 0)  && ! node->right.getPointer()->isInMCTSTree){
        if ((!node->right.getPointer()->isTerminal))
            return true;
        else
            return false;
    }
    if (!node->left.getPointer() && !node->right.getPointer()){
        // TODO any probleem with this? as no child
            node->isTerminal = 1;
        return false;
    }
    if (node->left.getPointer() != 0 && node->right.getPointer() != 0 && node->state == 0 &&
            node->left.getPointer()->state == 0 && node->right.getPointer()->state == 0) {
        return false;
    }
    
    if ((!node->isForked)){ 
        if (!node->isInMCTSTree)
            return true;
    }
    if (debug_thx) llvm::errs() << "--------------------------------------------------------- in hasEligiableChildren\n";
    if (0) {
      llvm::errs() << "--------------------------------------------------------- in hasEligiableChildren\n";
      llvm::errs() << "node : " << node << " node->state : " << node->state << "\n";
      llvm::errs() << "node->left : " << node->left.getPointer() << " node->right : " << node->right.getPointer() << "\n";
      llvm::errs() << "node->isInMCTSTree : " << node->isInMCTSTree << " node->isSimulated : " << node->isSimulated << " node->isTerminal" << node->isTerminal << "\n";
      if (node->left.getPointer()){
        llvm::errs() << "**** left : " << node->left.getPointer() << " ; left->state : " << node->left.getPointer()->state << "\n";
        llvm::errs() << "**** left isInMCTSTree : " << node->left.getPointer()->isInMCTSTree  << " isTerminal : "
            << node->left.getPointer()->isTerminal << " isSimulated : " << node->left.getPointer()->isSimulated << "\n";
      }
      if (node->right.getPointer()){
        llvm::errs() << "**** right : " << node->right.getPointer() << " ; right->state : " << node->right.getPointer()->state << "\n";
        llvm::errs() << "**** right isInMCTSTree : " << node->right.getPointer()->isInMCTSTree << " isTerminal : "
            << node->right.getPointer()->isTerminal << " isSimulated : " << node->right.getPointer()->isSimulated << "\n";
      }
      llvm::errs() << "---------------------------------------------------------\n";
    }
    return false; // if all children are visisted, should return false and let doSelection to select a child to continue
}

PTreeNode* MCTSSearcher::doSelection(PTreeNode* node){
    PTreeNode* selected_node = nullptr;
    if (node->left.getPointer() && node->right.getPointer() &&
            (node->left.getPointer()->isInMCTSTree) && (node->right.getPointer()->isInMCTSTree)){ // both two nodes can be selected
        selected_node = selectBestChildren(node, node->left.getPointer(), node->right.getPointer()); // TODO should select based on UCT value
        return selected_node;
    }
    // node (in subtree) will be detected and the value of node will turn to 0
    if (node->left.getPointer() && (node->left.getPointer()->isInMCTSTree)){ // the left node can be selected
        selected_node = node->left.getPointer();
        return selected_node;
    }
    if (node->right.getPointer() && (node->right.getPointer()->isInMCTSTree)){ // the right node can be selected
        selected_node = node->right.getPointer();
        return selected_node;
    }
  
    if (node->left.getPointer() == 0 && node->right.getPointer() != 0 && node->right.getPointer()->isTerminal){
        return node->right.getPointer();
    }
    if (node->right.getPointer() == 0 && node->left.getPointer() != 0 && node->left.getPointer()->isTerminal){
        return node->left.getPointer();
    }
    
    // handle switch statement
    if (node->left.getPointer() != 0 && node->right.getPointer() != 0 && node->state == 0 &&
            node->left.getPointer()->state == 0 && node->right.getPointer()->state == 0) {
        if(debug_thx) llvm::errs() << " Handling swith statement in doSelection ...\n";
        if (node->left.getPointer() == 0 || node->right.getPointer() == 0){
            llvm::errs() << "some thing error in handling switch statement in doSelection \n";
            exit(1);
        }
        if (node->left.getPointer()->isInMCTSTree == 0 || node->right.getPointer()->isInMCTSTree == 0){
            llvm::errs() << "some thing error (isInMCTSTree is not correctly marked) in handling switch statement in doSelection \n";
            llvm::errs() << "ptreeNode = " << executor.processTree << "\n";
            exit(1);
        }
        double left_reward = node->left.getPointer()->reward;
        double right_reward = node->right.getPointer()->reward;
        if (left_reward > right_reward) {
            return node->left.getPointer();
        }
        else if (left_reward < right_reward){
            return node->right.getPointer();
        }
        else {
            // randomly return in this case
            std::vector<PTreeNode*> vec_children = getChildren(node);
            assert(vec_children.size() == 2);
            return vec_children[theRNG.getInt32() % 2];
        }
    }
    //llvm::errs() << "Warning: miss corner cases in doSelection node = " << node << "\n";
    if (debug_thx && ! selectedNodeSet.count(node)) {
      llvm::errs() << "--------------------------------------------------------- in doSelection\n";
      llvm::errs() << "node : " << node << " node->state : " << node->state << "\n";
      llvm::errs() << "node->isInMCTSTree : " << node->isInMCTSTree << " node->isSimulated : " << node->isSimulated << " node->isTerminal: " << node->isTerminal << "\n";
      llvm::errs() << "node->left : " << node->left.getPointer() << " node->right : " << node->right.getPointer() << "\n";
      if (node->left.getPointer()){
        llvm::errs() << "**** left : " << node->left.getPointer() << " ; left->state : " << node->left.getPointer()->state << "\n";
        llvm::errs() << "**** left isInMCTSTree : " << node->left.getPointer()->isInMCTSTree  << " isTerminal : "
            << node->left.getPointer()->isTerminal << " isSimulated : " << node->left.getPointer()->isSimulated << "\n";
      }
      if (node->right.getPointer()){
        llvm::errs() << "**** right : " << node->right.getPointer() << " ; right->state : " << node->right.getPointer()->state << "\n";
        llvm::errs() << "**** right isInMCTSTree : " << node->right.getPointer()->isInMCTSTree << " isTerminal : "
            << node->right.getPointer()->isTerminal << " isSimulated : " << node->right.getPointer()->isSimulated << "\n";
      }
      llvm::errs() << "---------------------------------------------------------\n";
    }
    return node;
}

// get the child nodes by a given node
std::vector<PTreeNode*> MCTSSearcher::getChildren(PTreeNode* node){
    std::vector<PTreeNode*> ret_vec;
    assert(node != 0);
    assert(node->left.getPointer() != 0 && node->right.getPointer() !=0);
    ret_vec.push_back(node->left.getPointer());
    ret_vec.push_back(node->right.getPointer());
    return ret_vec;
}

// Compute UCT value for each child
double MCTSSearcher::computeUCT(PTreeNode* parent, PTreeNode* child){
    int bias_parameter = sqrt(2); //
    double quality = child->reward;
    unsigned int nu_child_visited = child->visit;
    unsigned int nu_parent_visited = parent->visit;
    double exploitation_score = (quality / nu_child_visited);
    double exploration_score = bias_parameter * sqrt(2 * log(nu_parent_visited) / nu_child_visited);
    double uct = exploration_score + exploitation_score;
    return uct;
}

// Select best children from UCT value
PTreeNode* MCTSSearcher::selectBestChildren(PTreeNode* current_node, PTreeNode* left, PTreeNode* right){
    PTreeNode* ret_node = nullptr;
    if (left  == 0 || right == 0){
        llvm::errs() << "Warning: something wrong when expandBestChildren\n";
        exit(1);
    }
    // select based on UCT value: no need to check the instructions, just check the reward
    if (0) llvm::errs() << "Debugging selectBestChildren ...\n";
    double uct_left = computeUCT(current_node, left);
    double uct_right = computeUCT(current_node, right);
    //if (debug_thx && ! selectedNodeSet.count(current_node)) llvm::errs() << "uct_left = " << uct_left << " uct_right =  " << uct_right << "\n";
    if (uct_left > uct_right) {
        ret_node = left;
    }
    else if (uct_left < uct_right){
        ret_node = right;
    }
    else {
        // randomly return in this case
        std::vector<PTreeNode*> vec_children = getChildren(current_node);
        assert(vec_children.size() == 2);
        ret_node = vec_children[theRNG.getInt32() % 2];
    }
    assert(ret_node != 0);
    return ret_node;
}

PTreeNode* MCTSSearcher::doExpansion(PTreeNode* node, std::vector<ExecutionState*> ){
    PTreeNode* expanded_node = nullptr;
    
    if (0) {
    //if (debug_thx && !expandedNodeSet.count(node)) {
      llvm::errs() << "---------------------------------------------------------\n";
      llvm::errs() << "node : " << node << " node->state : " << node->state << "\n";
      llvm::errs() << "node->isInMCTSTree : " << node->isInMCTSTree << " node->isSimulated : " << node->isSimulated << "node->isTerminal" << node->isTerminal << "\n";
      if (node->left.getPointer()){
        llvm::errs() << "**** left : " << node->left.getPointer() << " ; left->state : " << node->left.getPointer()->state << "\n";
        llvm::errs() << "**** left isInMCTSTree : " << node->left.getPointer()->isInMCTSTree  << " isTerminal : "
            << node->left.getPointer()->isTerminal << " isSimulated : " << node->left.getPointer()->isSimulated << "\n";
      }
      if (node->right.getPointer()){
        llvm::errs() << "**** right : " << node->right.getPointer() << " ; right->state : " << node->right.getPointer()->state << "\n";
        llvm::errs() << "**** right isInMCTSTree : " << node->right.getPointer()->isInMCTSTree << " isTerminal : "
            << node->right.getPointer()->isTerminal << " isSimulated : " << node->right.getPointer()->isSimulated << "\n";
      }
      llvm::errs() << "---------------------------------------------------------\n";
    }

    if (node->left.getPointer() && node->right.getPointer() && (!node->left.getPointer()->isInMCTSTree) && (!node->right.getPointer()->isInMCTSTree) &&
            node->left.getPointer()->state && node->right.getPointer()->state){
        if (debug_thx) llvm::errs() << "expansion if 1\n";
        
        // directly check for now
        expanded_node = expandBestChildren(node->left.getPointer(), node->right.getPointer()); // TODO should select based on point-to/alias information

        
        if (0) {
            llvm::errs() << "expanded node (before return) = " << expanded_node << " state " << expanded_node->state <<  "\n";
            if (node->left.getPointer()){
                llvm::errs() << "**** left : " << node->left.getPointer() << " ; left->state : " << node->left.getPointer()->state << "\n";
                llvm::errs() << "**** left isInMCTSTree : " << node->left.getPointer()->isInMCTSTree  << " isTerminal : "
                 << node->left.getPointer()->isTerminal << " isSimulated : " << node->left.getPointer()->isSimulated << "\n";
            }
            if (node->right.getPointer()){
                llvm::errs() << "**** right : " << node->right.getPointer() << " ; right->state : " << node->right.getPointer()->state << "\n";
                llvm::errs() << "**** right isInMCTSTree : " << node->right.getPointer()->isInMCTSTree << " isTerminal : "
                    << node->right.getPointer()->isTerminal << " isSimulated : " << node->right.getPointer()->isSimulated << "\n";
            }
        }
        return expanded_node;
        }
    if (node->left.getPointer() && (!node->left.getPointer()->isInMCTSTree) && node->left.getPointer()->state){ // the left node is expanedable
        if (debug_thx) llvm::errs() << "expansion if 2\n";
        expanded_node = node->left.getPointer();
        return expanded_node;
    }
    if (node->right.getPointer() && (!node->right.getPointer()->isInMCTSTree) && node->right.getPointer()->state){ // the right node is expanedable
        if (debug_thx) llvm::errs() << "expansion if 3\n";
        expanded_node = node->right.getPointer();
        return expanded_node;
    }
    // handle switch sttatement
    // main idea: never expand the node with state==0 but mark it as in MCTSTree, and let the doSelection to select a node based on the  reward
    if (node->left.getPointer() != 0 && node->right.getPointer() != 0 && node->state == 0 &&
            node->left.getPointer()->state == 0 && node->right.getPointer()->state == 0) {
        if (debug_thx) llvm::errs() << "Handling switch statement in doExpansion \n";
        if (node->left.getPointer()->state == 0 && node->right.getPointer()->state == 0){
            node->isInMCTSTree = 1;
            node->left.getPointer()->isInMCTSTree = 1;
            node->right.getPointer()->isInMCTSTree = 1;
            return node;
        }
        if (debug_thx) llvm::errs() << "Miss cases when handling swith statement !!!\n";
        exit(1);
    }
    if (!node->isForked ){ 
          
          if (!node->isInMCTSTree){ 
            if (debug_thx) llvm::errs() << "Still return current_state node in doExpansion " << node << " \n";
            return current_state->ptreeNode;
        }
    }
    // TODO do we miss any corner cases?
    if(debug_thx) llvm::errs() << "Warning: Some corner cases are missed in doExpansion node = " << node <<" \n";
    return node;
}

// get the BasicBlocks for branches so that we can decide to explore which branch first
// [0, 1]: 0 denotes left_state; 1 represents right_state
std::vector<std::vector<llvm::BasicBlock*>> MCTSSearcher::getBranchBlocks(ExecutionState* left_state, ExecutionState* right_state){
    std::vector<std::vector<llvm::BasicBlock*>> ret;
    std::vector<llvm::BasicBlock*> if_then_br, if_else_br;
    llvm::BasicBlock* left_pc_bb = left_state->pc->inst->getParent();
    llvm::BasicBlock* right_pc_bb = right_state->pc->inst->getParent();
    
    // Special handling 0: if the two basic block are not in the same function
    if (left_pc_bb->getParent() != right_pc_bb->getParent()){
        // TODO simpley return current block? check if this is common or not
        if_then_br.push_back(left_pc_bb);
        if_else_br.push_back(right_pc_bb);
        ret.push_back(if_then_br);
        ret.push_back(if_else_br);
        return ret;
    }
    llvm::PostDominatorTree PDT (*left_pc_bb->getParent());
    //llvm::DominatorTree DT(*bb1->getParent()); // same for the dominator usage

    // Step 1: find the first post-dominator
    llvm::BasicBlock* nearest_bb_post = PDT.findNearestCommonDominator(left_pc_bb, right_pc_bb);
    //llvm::errs() << "Nearest common postdominator: " << nearest_bb_post << "\n";

    // Special handling 1: the post-dominator do not exist
    if (nearest_bb_post == 0){
        // First attempt: get the the successors of left/right by levels and check if there is a common basic block
        int succ_level = 2; // TODO this succ_level controls how depth to find the postdominator;
        std::set<llvm::BasicBlock*> set_left {left_pc_bb};
        std::set<llvm::BasicBlock*> set_right {right_pc_bb};
        for (int i = 0; i < succ_level; i++){
        
            std::vector<llvm::BasicBlock*> vec_left_temp;
            for (auto bb : set_left){
                if (bb->getTerminator()->getNumSuccessors()){
                    for (succ_iterator start = succ_begin(bb), end = succ_end(bb); start != end; ++start){ // get all the successors of basic block bb
                        vec_left_temp.push_back(*start);
                    }
                }
                else
                    vec_left_temp.push_back(bb);
            }

            std::vector<llvm::BasicBlock*> vec_right_temp;
            for (auto bb : set_right){
                if (bb->getTerminator()->getNumSuccessors()){
                    for (succ_iterator start = succ_begin(bb), end = succ_end(bb); start != end; ++start){ // get all the successors of basic block bb
                        vec_right_temp.push_back(*start);
                    }
                }
                else
                    vec_right_temp.push_back(bb);
            }

            //find from left to right
            for (auto bb1 : vec_left_temp){
                for (auto bb2: vec_right_temp){
                    llvm::PostDominatorTree PDT (*bb1->getParent());
                    llvm::BasicBlock* nearest_bb = PDT.findNearestCommonDominator(bb1, bb2);
                    if (nearest_bb != 0){
                        nearest_bb_post = nearest_bb;
                        break;
                    }
                }
            }
            //find from right to left
            for (auto bb1 : vec_right_temp){
                for (auto bb2: vec_left_temp){
                    llvm::PostDominatorTree PDT (*bb1->getParent());
                    llvm::BasicBlock* nearest_bb = PDT.findNearestCommonDominator(bb1, bb2);
                    if (nearest_bb != 0){
                        nearest_bb_post = nearest_bb;
                        break;
                    }
                }
            }
            if (nearest_bb_post != 0)
                break;
            for (auto v1 : vec_left_temp)
                set_left.insert(v1);
            for (auto v2 : vec_right_temp)
                set_right.insert(v2);
        }
        // if we still can not find the postdominator, try the following solution: should make sense
        // The simplest implementation: just give out the current basic block as the postdominator can hardly to be found?
        if (nearest_bb_post == 0) { // if previous recursively finding does not work; let's see what we have missed?
            if (debug_thx) llvm::errs() << "PostDominator still not found, try to fix this case. Exit ????\n";
            //exit(1);
            for (auto bb1 : set_left){
                if_then_br.push_back(bb1);
            }
            for (auto bb2 : set_right) {
                if_else_br.push_back(bb2);
            }
            ret.push_back(if_then_br);
            ret.push_back(if_else_br);
            return ret;
        }
    }

    // The case where the postdominator is well found

    // Step 2: get the type of branch: if/for/switch?
    // For now is used the name of the block, any good ideas? TODO
    //llvm::Instruction* post_i = nearest_bb_post->getTerminator();
    std::string block_name = nearest_bb_post->getName().str();
    //llvm::errs() << "BasicBlock name : " << block_name << "\n";

    // Step 3: start from the basicblock pc points to, get successors util the post-dominator is encountered
    // assume this is if-then branch: the order of if-then/else should not matter
    // case 1: handle if branches
    //if (block_name.find("if.") != std::string::npos){
    std::stack<llvm::BasicBlock*> stack_left;
    std::stack<llvm::BasicBlock*> stack_right;
    // handle if-then branch
    stack_left.push(left_pc_bb);
    if_then_br.push_back(left_pc_bb);
    std::set<llvm::BasicBlock*> left_set;
    while (stack_left.size()){
        llvm::BasicBlock* tbb = stack_left.top();
        if (tbb == nearest_bb_post){ // check if this is the postdominator
            if_then_br.clear();
            break;
        }
        stack_left.pop();
        for (succ_iterator start = succ_begin(tbb), end = succ_end(tbb); start != end; ++start){ // get all the successors of basic block bb
            if (*start == nearest_bb_post){
                if (stack_left.size())
                    stack_left.pop(); // remove the top as it was added
                break;
            } else {
                if (std::find(if_then_br.begin(), if_then_br.end(), *start) == if_then_br.end()) // only store when it is not in the vector
                    if_then_br.push_back(*start);
                stack_left.push(*start);
                if (left_set.count(*start)){
                    // in the set
                    left_set.clear();
                    break;
                } else {
                    // not in the set
                    left_set.insert(*start);
                }
            }
        }
        if (left_set.size() == 0)
            break;
    }
    // handle if-then branch
    stack_right.push(right_pc_bb);
    if_else_br.push_back(right_pc_bb);
    std::set<llvm::BasicBlock*> right_set;
    while (stack_right.size()){
       llvm::BasicBlock* tbb = stack_right.top();
        if (tbb == nearest_bb_post){
            if_else_br.clear();
            break;
        }
        stack_right.pop();
        for (succ_iterator start = succ_begin(tbb), end = succ_end(tbb); start != end; ++start){ // get all the successors of basic block bb
            if (*start == nearest_bb_post){
                if (stack_right.size())
                    stack_right.pop(); // remove the top as it was added
                break;
            } else {
                if (std::find(if_else_br.begin(), if_else_br.end(), *start) == if_else_br.end()) // only store when it is not in the vector
                    if_else_br.push_back(*start);
                stack_right.push(*start);
                if (right_set.count(*start)){
                    // in the set
                    right_set.clear();
                    break;
                } else {
                    // not in the set
                    right_set.insert(*start);
                }
            }
        }
        if (right_set.size() == 0)
            break;
    }
   
    ret.push_back(if_then_br);
    ret.push_back(if_else_br);
    return ret;
}

std::vector<double> MCTSSearcher::getExpansionScore(std::vector<std::vector<llvm::BasicBlock*>> blocks, ExecutionState* left_state,
        ExecutionState* right_state, std::set<std::string> recordingSet){
    std::vector<double> ret;
    double left_expand_score = 0;
    double right_expand_score = 0;
    std::vector<llvm::BasicBlock*> bb_left_branch = blocks[0];
    std::vector<llvm::BasicBlock*> bb_right_branch = blocks[1];
    // TODO fast check if the blocks is empty because of no post-dominator were found
    if (blocks.size() != 2){
        assert(blocks.size() == 2);
        left_expand_score = 1;
        right_expand_score = 1;
        ret.push_back(left_expand_score);
        ret.push_back(right_expand_score);
        llvm::errs() << "EXIT at getExpansionScore?\n";
        exit(1);
        //return ret;
    }
    // now calulate the number of unsafe operations in left/right sides
    unsigned int count_left = 0;
    unsigned int count_right = 0;
    // TODO maintain a map to reduce the repeated searching: the value should be count or score?
    // handle bb_left_branch
    for (auto bb : bb_left_branch){
        unsigned int bb_count = 0;
        // find from map before checking the instruction one by one
        auto it = globalUnsafeBlockMap.find(bb);
        if (it != globalUnsafeBlockMap.end()) { // found it
            count_left += globalUnsafeBlockMap[bb];
            break;
        }
        for (auto i = bb->begin(), e = bb->end(); i != e; ++i){ // print instruction
            llvm::Instruction* ii = &*i;
            const llvm::DebugLoc* debug_info = &ii->getDebugLoc();
            if (debug_info->get()){
                if (isa<StoreInst>(ii)) {
                  
                    std::string record = "STORE-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if (debug_thx) llvm::errs() << "@@@@@@ records store: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_left += 1;
                        bb_count += 1;
                    }
                }
                if (isa<LoadInst>(ii)){
                   
                    std::string record = "LOAD-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if (debug_thx) llvm::errs() << "@@@@@@ records load: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_left += 1;
                        bb_count += 1;
                    }
                }
                if (isa<GetElementPtrInst>(ii)) {
                    
                    std::string record = "GETE-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if (debug_thx)  llvm::errs() << "@@@@@@@ records GetElementPtrInst: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_left += 1;
                        bb_count += 1;
                    }
                }
                if (isa<CastInst>(ii)) {
                    
                    std::string record = "CAST-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if(debug_thx) llvm::errs() << "@@@@@@ records cast: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_left += 1;
                        bb_count += 1;
                    }
                }
            }
        }
        // insert element to globalUnsafeBlockMap
        globalUnsafeBlockMap.insert(std::pair<llvm::BasicBlock*, unsigned int>(bb, bb_count));

    }
    // handle bb_right_branch
    for (auto bb : bb_right_branch){
        unsigned int bb_count = 0;
        // find from map before checking the instruction one by one
        auto it = globalUnsafeBlockMap.find(bb);
        if (it != globalUnsafeBlockMap.end()) { // found it
            count_right += globalUnsafeBlockMap[bb];
            break;
        }
        for (auto i = bb->begin(), e = bb->end(); i != e; ++i){ // print instruction
            llvm::Instruction* ii = &*i;
            const llvm::DebugLoc* debug_info = &ii->getDebugLoc();
            // TODO how to split the instruction from if-then-else/for-body-end/switch-case block?
            if (debug_info->get()){
                if (isa<StoreInst>(ii)) {
          
                    std::string record = "STORE-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if(debug_thx) llvm::errs() << "@@@@@@ records store: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_right += 1;
                        bb_count += 1;
                    }
                }
                if (isa<LoadInst>(ii)){
                   
                    std::string record = "LOAD-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if (debug_thx) llvm::errs() << "@@@@@@ records load: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_right += 1;
                        bb_count += 1;
                    }
                }
                if (isa<GetElementPtrInst>(ii)) {
                   
                    std::string record = "GETE-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if (debug_thx) llvm::errs() << "@@@@@@ records getelementptr: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_right += 1;
                        bb_count += 1;
                    }
                }
                if (isa<CastInst>(ii)) {
                   
                    std::string record = "CAST-" + ii->getFunction()->getName().str() + "_nesCheck"
                        + "-" + std::to_string(debug_info->getLine()) + "-" + std::to_string(debug_info->getCol());
                    if (debug_thx) llvm::errs() << "@@@@@@ records cast: " << record << "\n";
                    if (recordingSet.count(record)) { // the element is in the set, count is 1
                        count_right += 1;
                        bb_count += 1;
                    }
                }
            }
        }
        // insert element to globalUnsafeBlockMap
        globalUnsafeBlockMap.insert(std::pair<llvm::BasicBlock*, unsigned int>(bb, bb_count));
    }

    // TODO add points-to/alias analysis
    left_expand_score = 0.5 * count_left; // TODO will add more factors later
    right_expand_score = 0.5 * count_right;
    ret.push_back(left_expand_score);
    ret.push_back(right_expand_score);
    return ret;
}

PTreeNode* MCTSSearcher::expandBestChildren(PTreeNode* left, PTreeNode* right){
    if (left == 0 || right == 0){
        llvm::errs() << "Warning: something wrong when expandBestChildren\n";
        exit(1);
    }
    // expand the node based on pointer analysis
    ExecutionState* left_state = left->state;
    ExecutionState* right_state = right->state;
    
    std::vector<llvm::BasicBlock*> vec_bb;
    

    std::vector<std::vector<llvm::BasicBlock*>> ret = getBranchBlocks(left_state, right_state);
    if (debug_thx) llvm::errs() << "The vector (ret) size = " << ret.size() << "\n";

    std::vector<double> ret_expansion_score = getExpansionScore(ret, left_state, right_state, globalRecordingSet);
    if (debug_thx) llvm::errs() << "The vector (ret_expansion_score) size = " << ret_expansion_score.size() << "\n";

    double score_left = ret_expansion_score[0];
    double score_right = ret_expansion_score[1];
    if (debug_thx) llvm::errs() << "!!! left expansion score : " << score_left << "\n";
    if (debug_thx) llvm::errs() << "!!! right expansion score : " << score_right << "\n";
    if (score_left > score_right){
        if (debug_thx) llvm::errs() << "!!! Expand left node (score_left > score_right) left = " << left << "\n";
        return left;
    } else if (score_left < score_right){
        if (debug_thx) llvm::errs() << "!!! Expand right node (score_left < score_right) right = " << right << "\n";
        return right;
    } else {
       if (debug_thx) llvm::errs() << "!!! Expand left node (score_left == score_right) left = " << left << "\n";
       // randomly return in this case
       std::vector<PTreeNode*> vec_children;
       vec_children.push_back(left);
       vec_children.push_back(right);
       assert(vec_children.size() == 2);
       return vec_children[theRNG.getInt32() % 2];
    }

}

// use the same logic as Executor.cpp::run
// TODO
// @1 how to change the search & update states
// @2 how to make this process spearately with normal execution: PTree::remove
double MCTSSearcher::doSimulation(PTreeNode* node){
    //llvm::errs() << "////////////// (doSimulation Done) ...\n";
    double ret = 0;
    if (node->state == 0){
        llvm::errs() << "Simulation on a NULL ExecutionState!!!\n";
        exit(1); // should never happen
    }
    is_simulation_mode = 1; // TODO do we really need two flgs for this?
    
    if (debug_thx) llvm::errs() << "////////////// (doSimulation Run Start) for " << node->state->ptreeNode << " ...\n";

    executor.suspendState(*node->state);

    // TODO
    // step 1: create a snapshot here, so we can switch to the normal execution
    //*node->state->setSimulationState(&*node->state);
    /* initialize... */
    SimulationInfo *simulationInfo = new SimulationInfo();
    Snapshot *snapshot = new Snapshot();
    snapshot->state = node->state;
    simulationInfo->snapshot = snapshot;
    executor.startSimulationState(*node->state, simulationInfo);
    node->isSimulated = 1;
    ret = executor.simulationReward;
    return ret;
}

MCTSSearcher::MCTSSearcher(PTree &processTree, Executor &_executor, RNG &rng)
  : processTree{processTree}, executor(_executor), theRNG{rng}
    {
        if (debug_thx) llvm::errs() << "Init!!! Storing the file information to KLEE\n";
        if (debug_thx){
        for (std::set<std::string>::iterator it=globalRecordingSet.begin(); it!=globalRecordingSet.end(); ++it)
            llvm::errs() << *it << "\n";
        if(debug_thx) llvm::errs() << "size of globalRecordingSet = " << globalRecordingSet.size() << "\n";
        }
        /// THX end
    };
std::set<PTreeNode* > rootNodeSet;
ExecutionState &MCTSSearcher::selectState(){
    ExecutionState* selected_state = nullptr;
    PTreeNode* selected_node = nullptr;
    PTreeNode* expanded_node = nullptr;
   
    if (children.size() == 0) {
        llvm::errs() << "No children is avaiable now, so exit ... executor.states.size() " << executor.states.size() << "\n";
        exit(1);
    }

    if (children.size() == 1){
        selected_state = children[0];
    } else{
        // There are more than one node, then use MCTS strategy
        PTreeNode* root = processTree.root.getPointer();
        // TODO remove repeated selection and expansion from root due to state switch (check if it is necessary)
        while (! isTerminalNode(root)) { // start from root
            if (debug_thx && !rootNodeSet.count(root)) llvm::errs() << "----------- While-loop root = " << root  << " root->isForked = " << root->isForked << ", isTerminal = "
             << root->isTerminal << " , isInMCTSTree = " << root->isInMCTSTree << " root->state = " << root->state <<"\n";
            rootNodeSet.insert(root);
            if (!hasEligiableChildren(root)) { // all branches are alread expanded for *true*
                selected_node = doSelection(root);
                root = selected_node;
                if ((!root->left.getPointer()) && (!root->right.getPointer())){
                    return *selected_node->state;
                 
                }

                if (debug_thx & ! selectedNodeSet.count(selected_node)) llvm::errs() << "------------ (doSelection Done) selected_node = " << selected_node << "\n";
                if (root->isTerminal)
                    continue;

                // TODO what if all the nodes are explored?
                selectedNodeSet.insert(root);
            }
            else {
                
                expanded_node = doExpansion(root, children);

                if (expanded_node->state == 0 && debug_thx) { // TODO why this is 0? may be because the simulation run termination is not clean?
                    if (debug_thx) llvm::errs() << "expanded_node with state 0 ? " << "; expanded_node = " << expanded_node << "; expanded_node->state = "
                        << expanded_node->state << "\n";
                    if (expanded_node->left.getPointer()){
                        llvm::errs() << "expanded_node->left node : " << expanded_node->left.getPointer() << "\n";
                        llvm::errs() << "   :->isInMCTSTree : " << expanded_node->left.getPointer()->isInMCTSTree << " node->isSimulated : " << expanded_node->left.getPointer()->isSimulated <<
                            " node->isTerminal: " <<expanded_node->left.getPointer()->isTerminal << " state " << expanded_node->left.getPointer()->state <<"\n";
                    }
                    if (expanded_node->right.getPointer()){
                        llvm::errs() << "expanded_node->right node : " << expanded_node->right.getPointer() << "\n";
                        llvm::errs() << "   :->isInMCTSTree : " << expanded_node->right.getPointer()->isInMCTSTree << " node->isSimulated : " << expanded_node->right.getPointer()->isSimulated <<
                            " node->isTerminal: " <<expanded_node->right.getPointer()->isTerminal << " state = " << expanded_node->right.getPointer()->state << "\n";
                     }
                }

                if (debug_thx) llvm::errs() << " *********  (doExpansion) expanded_node =" << expanded_node << " state " << expanded_node->state <<  "\n";
                if (debug_thx) {
                  llvm::errs() << "************ (doExpansion Done) While-loop else expanded_node = " << expanded_node << "...\n";
                  printf("isInMCTSTree = %d\n", expanded_node->isInMCTSTree);
                  llvm::errs() << "isInMCTSTree = " << expanded_node->isInMCTSTree << "; isTerminal = " << expanded_node->isTerminal <<
                      "; isSimulated = " << expanded_node->isSimulated << "; visit = " << expanded_node->visit << "\n";
                  llvm::errs() << "             expanded state isTerminal =" << expanded_node->isTerminal << "\n";
                  if (expanded_node->state)
                    llvm::errs() << "             expanded state isNomalState  =" << expanded_node->state->isNormalState() << "\n";
                }
                //exit(1);
                // TODO check if it simulates many times
                // perform simuation
                selected_state = expanded_node->state;
                if (selected_state == 0){ // handle special switch statement
                    if (expanded_node->left.getPointer() && expanded_node->right.getPointer()){
                        expanded_node->left.getPointer()->isInMCTSTree = 1;
                        expanded_node->right.getPointer()->isInMCTSTree = 1;
                        continue;
                    }
                    if (expanded_node->left.getPointer()){
                        if (debug_thx && ! expandedNodeSet.count(expanded_node)) llvm::errs() << "expanded left ? =" << expanded_node->left.getPointer()
                            << " state = " << expanded_node->left.getPointer()->state << "\n";
                        expanded_node->left.getPointer()->isInMCTSTree = 1;
                        continue;
                    }
                    if (expanded_node->right.getPointer()){
                        if (debug_thx && !expandedNodeSet.count(expanded_node)) llvm::errs() << "expanded right ? =" << expanded_node->right.getPointer()
                            << " state = " << expanded_node->right.getPointer()->state << "\n";
                        expanded_node->right.getPointer()->isInMCTSTree = 1;
                        continue;
                    }
                    if (! expanded_node->left.getPointer() && ! expanded_node->right.getPointer()){
                        llvm::errs() << " (unexpected) goes here during expansion? " << " processTree = " << executor.processTree << "\n";
                        exit(1);
                    }
                }
                expanded_node->isInMCTSTree = 1;
                if (expanded_node->isTerminal == 1){
                    return *current_state;
                }
                current_state = selected_state;
                if (!expanded_node->state->isNormalState() && expanded_node->state->isSimulationState()){
                    if (expanded_node->left.getPointer() == 0 && expanded_node->right.getPointer() == 0) {
                        
                        continue;
                    }else {
                        llvm::errs() << "Corner case here: should never happen\n";
                        exit(1);
                    }
               }

                if (expanded_node->isTerminal){
                    llvm::errs() << "Corner case here: should never happen\n";
                    exit(1);
                    continue;
                }
                expandedNodeSet.insert(expanded_node);
                if (enable_simulation){
                    if (!expanded_node->isSimulated){
                        if (simulation_opt){
                            // perform optimization to aviod repeat simulating
                            llvm::BasicBlock* bb = expanded_node->state->pc->inst->getParent();
                            std::string bb_name = bb->getName().str();
                            if(executor.simulatedHistory.find(bb_name) == executor.simulatedHistory.end()){ // not find
                                std::vector<double> record;
                                record.push_back(0); // reward
                                record.push_back(0); // repeated time
                                record.push_back(0); // visited time
                                executor.simulatedHistory[bb_name] = record;
                            } else {
                                executor.simulatedHistory[bb_name][2] += 1;
                            }
                            if (debug_thx) llvm::errs() << "name: " << bb_name << "value: " << executor.simulatedHistory[bb_name][0] << " " <<
                                executor.simulatedHistory[bb_name][1] << " " << executor.simulatedHistory[bb_name][2] << "\n";
                            if (executor.simulatedHistory[bb_name][1] < simulation_limit
                                ){ // only perform the simulation when there are no new values after xx trials
                                double reward = doSimulation(expanded_node);
                                // update simulatedHistory
                                if ( executor.simulatedHistory[bb_name][0] >=  reward){
                                    executor.simulatedHistory[bb_name][1] += 1;
                                } else{
                                    executor.simulatedHistory[bb_name][1] = 0;
                                if ( reward > executor.simulatedHistory[bb_name][0]) // only store the maxinum reward
                                    executor.simulatedHistory[bb_name][0] = reward;
                                }
                            } else {
                                expanded_node->isSimulated = 1;
                                executor.terminateState(*selected_state);
                                continue; 
                            }
                        }
                        else { // without simulation optimization: just simulate every state
                            doSimulation(expanded_node);
                        }
                    }
                }
                break;
            }
        }
    }
    return *selected_state;
}




void MCTSSearcher::update(ExecutionState *current,
                                const std::vector<ExecutionState *> &addedStates,
                                const std::vector<ExecutionState *> &removedStates) {
  
  // THX store the current state
  if (current)
    current->ptreeNode->isInMCTSTree = 0;
  current_state = current;

  // insert states
  for (auto es : addedStates) {
    //add state to the children_states
    if (addedStates.size() == 1 && es->isNormalState() && current == NULL){
        current_state = es;
    }
    children.push_back(es);
    
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;

    PTreeNodePtr *childPtr;

    childPtr = parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                              : &parent->right)
                      : &processTree.root;
    while (pnode && !IS_OUR_NODE_VALID_MCTS(*childPtr)) {
      childPtr->setInt(childPtr->getInt());
      pnode = parent;
      if (pnode)
        parent = pnode->parent;

      childPtr = parent
                     ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                             : &parent->right)
                     : &processTree.root;
    }
  }

  // remove states
  for (auto es : removedStates) {
    //llvm::errs() << "--- MCTSSearcher::update removedStates.size() = " << removedStates.size() << "\n";
    if (debug_thx){
        llvm::errs() << "THX removedStates is not NULL" << ", and the Node is " << es->ptreeNode << "\n";
        
    }
    // THX remove children
        if (es ==children.front()) {
            children.erase(children.begin());
        } else {
            auto it = std::find(children.begin(), children.end(), es);
            assert(it != children.end() && "invalid state removed");
            children.erase(it); // remove it from children
        }
    PTreeNode *pnode = es->ptreeNode, *parent = pnode->parent;

    while (pnode && !IS_OUR_NODE_VALID_MCTS(pnode->left) &&
           !IS_OUR_NODE_VALID_MCTS(pnode->right)) {
      auto childPtr =
          parent ? ((parent->left.getPointer() == pnode) ? &parent->left
                                                         : &parent->right)
                 : &processTree.root;
      assert(IS_OUR_NODE_VALID_MCTS(*childPtr) && "Removing pTree child not ours");
      childPtr->setInt(childPtr->getInt());
      pnode = parent;
      if (pnode)
        parent = pnode->parent;
    }
  }
}

bool MCTSSearcher::empty() {
  return !IS_OUR_NODE_VALID_MCTS(processTree.root);
}

void MCTSSearcher::printName(llvm::raw_ostream &os) {
  llvm::errs() << "++++++ This is in MCTSSearcher\n";
  os << "MCTSSearcher\n";
}

// THX MCTS implementation end
///
//
/* splitted searcher */
SplittedSearcher::SplittedSearcher(Searcher *mctsSearcher, Searcher *simulationSearcher, unsigned int sim_mode)
  : mctsSearcher(mctsSearcher), simulationSearcher(simulationSearcher), sim_mode(sim_mode)
{

}

SplittedSearcher::~SplittedSearcher() {
  delete mctsSearcher;
}

ExecutionState &SplittedSearcher::selectState() {
  //if (mctsSearcher->empty()) {
  if (! is_simulation_mode) {
    /* the simulation states are supposed to be not empty */
    //if (1) llvm::errs() << "--- Goes to mctsSearcher\n";
    return mctsSearcher->selectState();
  }

  if (is_simulation_mode) {
  //if (simulationSearcher->empty()) {
    /* the mcts searcher is supposed to be not empty */
    //if (1) llvm::errs() << "--- Goes to simulationSearcher\n";
    return simulationSearcher->selectState();
  }
  llvm::errs() << "No seaercher was found!!!";
  exit(1);
  /* in this case, both searchers are supposed to be not empty */
  // we should keep simulation until one state is terminated
  /* we handle simulation states in a random manner */
  //return simulationSearcher->selectState();
}

void SplittedSearcher::update(
  ExecutionState *current,
  const std::vector<ExecutionState *> &addedStates,
  const std::vector<ExecutionState *> &removedStates
) {
  std::vector<ExecutionState *> addedOriginatingStates;
  std::vector<ExecutionState *> addedSimulationStates;
  std::vector<ExecutionState *> removedOriginatingStates;
  std::vector<ExecutionState *> removedSimulationStates;

  /* split added states */
  for (auto i = addedStates.begin(); i != addedStates.end(); i++) {
    ExecutionState *es = *i;
    if (es->isSimulationState()) {
      addedSimulationStates.push_back(es);
    } else {
      addedOriginatingStates.push_back(es);
    }
  }

  /* split removed states */
  for (auto i = removedStates.begin(); i != removedStates.end(); i++) {
    ExecutionState *es = *i;
    if (es->isSimulationState()) {
      removedSimulationStates.push_back(es);
    } else {
      removedOriginatingStates.push_back(es);
    }
  }

  if (current && current->isSimulationState()) {
    mctsSearcher->update(NULL, addedOriginatingStates, removedOriginatingStates);
  } else {
    mctsSearcher->update(current, addedOriginatingStates, removedOriginatingStates);
  }

  if (current && !current->isSimulationState()) {
    simulationSearcher->update(NULL, addedSimulationStates, removedSimulationStates);
  } else {
    simulationSearcher->update(current, addedSimulationStates, removedSimulationStates);
  }
}

bool SplittedSearcher::empty() {
  return mctsSearcher->empty() && simulationSearcher->empty();
}

/* random-path searcher for the simulation model */
RandomSimulationPath::RandomSimulationPath(Executor &executor)
  : executor(executor)
{

}

RandomSimulationPath::~RandomSimulationPath() {

}

ExecutionState &RandomSimulationPath::selectState() {
  llvm::errs() << "TreeStack is empty = " << treeStack.empty() << "\n";
  llvm::errs() << "--- Size of states in simulation run = " << states.size() << "\n";
  if (states.size() == 1) {
    /* as this point, the order of selection does not matter */
    return *states.front();
  }

  unsigned int flips = 0;
  unsigned int bits = 0;

  /* select the root */
  PTree::Node *n = treeStack.top();
  llvm::errs() << " the root n = " << n << "\n";

  while (!n->state) {
    if (!n->left.getPointer()) {
      n = n->right.getPointer();
    } else if (!n->right.getPointer()) {
      n = n->left.getPointer();
    } else {
      if (bits==0) {
        flips = theRNG.getInt32();
        bits = 32;
      }
      --bits;
      n = (flips&(1<<bits)) ? n->left.getPointer() : n->right.getPointer();
    }
  }

  ExecutionState *es = n->state;
  while (es->isSuspended()) {
    es = es->getSimulationState();
  }
  return *es;
}

void RandomSimulationPath::update(
  ExecutionState *current,
  const std::vector<ExecutionState *> &addedStates,
  const std::vector<ExecutionState *> &removedStates
) {
  llvm::errs() << "--- RandomSimulationPath::update executed ... \n";
  for (auto i = addedStates.begin(); i != addedStates.end(); i++) {
    ExecutionState *es = *i;
    if (es->getLevel() == treeStack.size()) {
      /* this state has a higher level, so we push it as a root */
      treeStack.push(es->ptreeNode);
    }

    /* add state */
    states.push_back(es);
  }
  for (auto i = removedStates.begin(); i != removedStates.end(); i++) {
    ExecutionState *es = *i;
    /* a top level recovery state terminated, so we pop it's root from the stack */
    if (es->isResumed() && es->getLevel() == treeStack.size() - 1) {
      treeStack.pop();
    }

    /* remove state */
    for (auto j = states.begin(); j != states.end(); j++) {
      if (es == *j) {
        states.erase(j);
        break;
      }
    }
  }
}

bool RandomSimulationPath::empty() {
  return treeStack.empty() && states.empty();
}


MergingSearcher::MergingSearcher(Searcher *baseSearcher)
  : baseSearcher{baseSearcher} {};

void MergingSearcher::pauseState(ExecutionState &state) {
  assert(std::find(pausedStates.begin(), pausedStates.end(), &state) == pausedStates.end());
  pausedStates.push_back(&state);
  baseSearcher->update(nullptr, {}, {&state});
}

void MergingSearcher::continueState(ExecutionState &state) {
  auto it = std::find(pausedStates.begin(), pausedStates.end(), &state);
  assert(it != pausedStates.end());
  pausedStates.erase(it);
  baseSearcher->update(nullptr, {&state}, {});
}

ExecutionState& MergingSearcher::selectState() {
  assert(!baseSearcher->empty() && "base searcher is empty");

  if (!UseIncompleteMerge)
    return baseSearcher->selectState();

  // Iterate through all MergeHandlers
  for (auto cur_mergehandler: mergeGroups) {
    // Find one that has states that could be released
    if (!cur_mergehandler->hasMergedStates()) {
      continue;
    }
    // Find a state that can be prioritized
    ExecutionState *es = cur_mergehandler->getPrioritizeState();
    if (es) {
      return *es;
    } else {
      if (DebugLogIncompleteMerge){
        llvm::errs() << "Preemptively releasing states\n";
      }
      // If no state can be prioritized, they all exceeded the amount of time we
      // are willing to wait for them. Release the states that already arrived at close_merge.
      cur_mergehandler->releaseStates();
    }
  }
  // If we were not able to prioritize a merging state, just return some state
  return baseSearcher->selectState();
}

void MergingSearcher::update(ExecutionState *current,
                             const std::vector<ExecutionState *> &addedStates,
                             const std::vector<ExecutionState *> &removedStates) {
  // We have to check if the current execution state was just deleted, as to
  // not confuse the nurs searchers
  if (std::find(pausedStates.begin(), pausedStates.end(), current) == pausedStates.end()) {
    baseSearcher->update(current, addedStates, removedStates);
  }
}

bool MergingSearcher::empty() {
  return baseSearcher->empty();
}

void MergingSearcher::printName(llvm::raw_ostream &os) {
  os << "MergingSearcher\n";
}


///

BatchingSearcher::BatchingSearcher(Searcher *baseSearcher,
                                   time::Span timeBudget,
                                   unsigned instructionBudget)
    : baseSearcher{baseSearcher}, timeBudgetEnabled{timeBudget},
      timeBudget{timeBudget}, instructionBudgetEnabled{instructionBudget > 0},
      instructionBudget{instructionBudget} {};

bool BatchingSearcher::withinTimeBudget() const {
  return !timeBudgetEnabled ||
         (time::getWallTime() - lastStartTime) <= timeBudget;
}

bool BatchingSearcher::withinInstructionBudget() const {
  return !instructionBudgetEnabled ||
         (stats::instructions - lastStartInstructions) <= instructionBudget;
}

ExecutionState &BatchingSearcher::selectState() {
  if (lastState && withinTimeBudget() && withinInstructionBudget()) {
    // return same state for as long as possible
    return *lastState;
  }

  // ensure time budget is larger than time between two calls (for same state)
  if (lastState && timeBudgetEnabled) {
    time::Span delta = time::getWallTime() - lastStartTime;
    auto t = timeBudget;
    t *= 1.1;
    if (delta > t) {
      klee_message("increased time budget from %f to %f\n",
                   timeBudget.toSeconds(), delta.toSeconds());
      timeBudget = delta;
    }
  }

  // pick a new state
  lastState = &baseSearcher->selectState();
  if (timeBudgetEnabled) {
    lastStartTime = time::getWallTime();
  }
  if (instructionBudgetEnabled) {
    lastStartInstructions = stats::instructions;
  }
  return *lastState;
}

void BatchingSearcher::update(ExecutionState *current,
                              const std::vector<ExecutionState *> &addedStates,
                              const std::vector<ExecutionState *> &removedStates) {
  // drop memoized state if it is marked for deletion
  if (std::find(removedStates.begin(), removedStates.end(), lastState) != removedStates.end())
    lastState = nullptr;
  // update underlying searcher
  baseSearcher->update(current, addedStates, removedStates);
}

bool BatchingSearcher::empty() {
  return baseSearcher->empty();
}

void BatchingSearcher::printName(llvm::raw_ostream &os) {
  os << "<BatchingSearcher> timeBudget: " << timeBudget
     << ", instructionBudget: " << instructionBudget
     << ", baseSearcher:\n";
  baseSearcher->printName(os);
  os << "</BatchingSearcher>\n";
}


///

IterativeDeepeningTimeSearcher::IterativeDeepeningTimeSearcher(Searcher *baseSearcher)
  : baseSearcher{baseSearcher} {};

ExecutionState &IterativeDeepeningTimeSearcher::selectState() {
  ExecutionState &res = baseSearcher->selectState();
  startTime = time::getWallTime();
  return res;
}

void IterativeDeepeningTimeSearcher::update(ExecutionState *current,
                                            const std::vector<ExecutionState *> &addedStates,
                                            const std::vector<ExecutionState *> &removedStates) {

  const auto elapsed = time::getWallTime() - startTime;

  // update underlying searcher (filter paused states unknown to underlying searcher)
  if (!removedStates.empty()) {
    std::vector<ExecutionState *> alt = removedStates;
    for (const auto state : removedStates) {
      auto it = pausedStates.find(state);
      if (it != pausedStates.end()) {
        pausedStates.erase(it);
        alt.erase(std::remove(alt.begin(), alt.end(), state), alt.end());
      }
    }
    baseSearcher->update(current, addedStates, alt);
  } else {
    baseSearcher->update(current, addedStates, removedStates);
  }

  // update current: pause if time exceeded
  if (current &&
      std::find(removedStates.begin(), removedStates.end(), current) == removedStates.end() &&
      elapsed > time) {
    pausedStates.insert(current);
    baseSearcher->update(nullptr, {}, {current});
  }

  // no states left in underlying searcher: fill with paused states
  if (baseSearcher->empty()) {
    time *= 2U;
    klee_message("increased time budget to %f\n", time.toSeconds());
    std::vector<ExecutionState *> ps(pausedStates.begin(), pausedStates.end());
    baseSearcher->update(nullptr, ps, std::vector<ExecutionState *>());
    pausedStates.clear();
  }
}

bool IterativeDeepeningTimeSearcher::empty() {
  return baseSearcher->empty() && pausedStates.empty();
}

void IterativeDeepeningTimeSearcher::printName(llvm::raw_ostream &os) {
  os << "IterativeDeepeningTimeSearcher\n";
}


///

InterleavedSearcher::InterleavedSearcher(const std::vector<Searcher*> &_searchers) {
  searchers.reserve(_searchers.size());
  for (auto searcher : _searchers)
    searchers.emplace_back(searcher);
}

ExecutionState &InterleavedSearcher::selectState() {
  Searcher *s = searchers[--index].get();
  if (index == 0) index = searchers.size();
  return s->selectState();
}

void InterleavedSearcher::update(ExecutionState *current,
                                 const std::vector<ExecutionState *> &addedStates,
                                 const std::vector<ExecutionState *> &removedStates) {

  // update underlying searchers
  for (auto &searcher : searchers)
    searcher->update(current, addedStates, removedStates);
}

bool InterleavedSearcher::empty() {
  return searchers[0]->empty();
}

void InterleavedSearcher::printName(llvm::raw_ostream &os) {
  os << "<InterleavedSearcher> containing " << searchers.size() << " searchers:\n";
  for (const auto &searcher : searchers)
    searcher->printName(os);
  os << "</InterleavedSearcher>\n";
}
