/*
 * Copyright (c) 2021, WSO2 Inc. (http://www.wso2.org) All Rights Reserved.
 *
 * WSO2 Inc. licenses this file to you under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "MapInsns.h"
#include "CodeGenUtils.h"
#include "Function.h"
#include "Operand.h"
#include "Package.h"
#include "TypeUtils.h"
#include "Types.h"
#include "Variable.h"

using namespace std;

namespace nballerina {

// new Map Instruction and Codegen logic are in the llvmStructure.cpp

MapStoreInsn::MapStoreInsn(const Operand &lhs, BasicBlock &currentBB, const Operand &KOp, const Operand &rOp)
    : NonTerminatorInsn(lhs, currentBB), keyOp(KOp), rhsOp(rOp) {}

void MapStoreInsn::translate(llvm::Module &module, llvm::IRBuilder<> &builder) {
    const auto &funcObj = getFunctionRef();
    const auto &lhsVar = funcObj.getLocalOrGlobalVariable(getLhsOperand());
    auto memberTypeTag = lhsVar.getType().getMemberTypeTag();
    TypeUtils::checkMapSupport(memberTypeTag);
    llvm::Value *mapValue = Type::isSmartStructType(memberTypeTag) ? funcObj.getLLVMLocalOrGlobalVar(rhsOp, module)
                                                                   : funcObj.createTempVariable(rhsOp, module, builder);
    builder.CreateCall(CodeGenUtils::getMapStoreFunc(module, memberTypeTag),
                       llvm::ArrayRef<llvm::Value *>({funcObj.createTempVariable(getLhsOperand(), module, builder),
                                                      funcObj.createTempVariable(keyOp, module, builder), mapValue}));
}

MapLoadInsn::MapLoadInsn(const Operand &lhs, BasicBlock &currentBB, const Operand &KOp, const Operand &rOp)
    : NonTerminatorInsn(lhs, currentBB), keyOp(KOp), rhsOp(rOp) {}

void MapLoadInsn::translate(llvm::Module &module, llvm::IRBuilder<> &builder) {
    const auto &funcObj = getFunctionRef();
    TypeTag memTypeTag = funcObj.getLocalOrGlobalVariable(rhsOp).getType().getMemberTypeTag();
    auto *outParamType = CodeGenUtils::getLLVMTypeOfType(memTypeTag, module);

    auto *lhs = funcObj.getLLVMLocalOrGlobalVar(getLhsOperand(), module);
    auto *outParam = builder.CreateAlloca(outParamType);
    auto *rhsTemp = funcObj.createTempVariable(rhsOp, module, builder);
    auto *keyTemp = funcObj.createTempVariable(keyOp, module, builder);
    auto mapLoadFunction = CodeGenUtils::getMapLoadFunc(module, memTypeTag);

    [[maybe_unused]] auto *retVal =
        builder.CreateCall(mapLoadFunction, llvm::ArrayRef<llvm::Value *>({rhsTemp, keyTemp, outParam}));
    // TODO check retVal and branch
    // if retVal is true
    if (Type::isSmartStructType(memTypeTag)) {
        auto *outParamTemp = builder.CreateLoad(outParam);
        builder.CreateStore(outParamTemp, lhs);
    } else {
        getPackageMutableRef().storeValueInSmartStruct(module, builder, outParam, Type(memTypeTag, ""), lhs);
    }
    // else
    // getFunctionMutableRef().storeValueInSmartStruct(modRef, getPackageRef().getGlobalNilVar(), Type(TYPE_TAG_NIL,
    // ""), lhs);
}

} // namespace nballerina
