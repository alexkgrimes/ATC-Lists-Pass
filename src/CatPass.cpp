#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Value.h"

#include "Noelle.hpp"

using namespace llvm::noelle ;

namespace {

  struct CAT : public ModulePass {
    static char ID; 

    CAT() : ModulePass(ID) {}

    bool doInitialization (Module &M) override {
      return false;
    }

    bool runOnModule (Module &M) override {

      /*
       * Fetch NOELLE
       */
      auto& noelle = getAnalysis<Noelle>();

      /*
       * Use NOELLE
       */
      // auto fm = noelle.getFunctionsManager();
      // auto mainF = fm->getEntryFunction();

      auto loopStructures = noelle.getLoopStructures();


      for (auto LS : *loopStructures) {
        if (isListLoop(LS)) {
          errs() << "found the pattern\n";
        }
      }


      return false;
    }

    bool isNodePointer(Value* v) {
      auto vType = v->getType();
      if (auto vPointerType = dyn_cast<llvm::PointerType>(vType)) { 
        auto structName = vPointerType->getElementType()->getStructName();
        if (structName != "struct.NodeData") {
          return false;
        }
      } else {
        return false;
      } 

      return true;
    }

    bool isListLoop(LoopStructure* LS) {
      /*
        * Print the first instruction the loop executes.
        */
      // auto LS = loop->getLoopStructure();
      auto entryInst = LS->getEntryInstruction();
      errs() << "Loop " << *entryInst << "\n";

      auto header = LS->getHeader();

      for (auto &inst : *header) {
        errs() << inst << "\n";

        if (auto cmpInst = dyn_cast<CmpInst>(&inst)) {
          if (cmpInst->getPredicate() == llvm::CmpInst::ICMP_NE) {
            Value* lhs = cmpInst->getOperand(0);
            Value* rhs = cmpInst->getOperand(1);

            // Verify types of the comparison 
            if (!((isNodePointer(lhs) && isa<llvm::ConstantPointerNull>(rhs)) || 
                 (isNodePointer(rhs) && isa<llvm::ConstantPointerNull>(lhs)))) {
              return false;
            }

            // Get variable for Node in comparison
            Instruction* inst;
            if (Instruction* nodeInst = dyn_cast<Instruction>(lhs)) {
              inst = nodeInst;
            }
            if (Instruction* nodeInst = dyn_cast<Instruction>(rhs)) {
              inst = nodeInst;
            }

            bool definedByListFront = false;
            bool isUpdatedWithNext = false;
            if (auto loadInst = dyn_cast<LoadInst>(inst)) {
              auto pointerOperand = loadInst->getPointerOperand();

              for (auto user : pointerOperand->users()) {
                if (auto storeInst = dyn_cast<StoreInst>(user)) {
                  auto storedValue = storeInst->getOperand(0);
                  
                  if (auto listFrontCall = dyn_cast<CallInst>(storedValue)) {
                    if (listFrontCall->getCalledFunction()->getName() == "List_front") {
                      definedByListFront = true;
                    }  

                    if (listFrontCall->getCalledFunction()->getName() == "Node_next") {
                      isUpdatedWithNext = true;
                    } 
                  }
                }
              }
            }

            if (!definedByListFront || !isUpdatedWithNext) {
              return false;
            }
          }
        }
      }

      return true;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<Noelle>();
    }
  };

}

// Next there is code to register your pass to "opt"
char CAT::ID = 0;
static RegisterPass<CAT> X("CAT", "Simple user of the Noelle framework");

// Next there is code to register your pass to "clang"
static CAT * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new CAT()); }}); // ** for -O0
