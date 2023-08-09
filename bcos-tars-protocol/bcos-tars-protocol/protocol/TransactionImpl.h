/**
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief tars implementation for Transaction
 * @file TransactionImpl.h
 * @author: ancelmo
 * @date 2021-04-20
 */

#pragma once

#include "../impl/TarsHashable.h"

#include "bcos-concepts/ByteBuffer.h"
#include "bcos-concepts/Hash.h"
#include "bcos-tars-protocol/tars/Transaction.h"
#include <bcos-crypto/hasher/Hasher.h>
#include <bcos-crypto/interfaces/crypto/CommonType.h>
#include <bcos-framework/protocol/Transaction.h>
#include <bcos-utilities/Common.h>
#include <bcos-utilities/DataConvertUtility.h>
#include <memory>

namespace bcostars::protocol
{

class TransactionImpl : public bcos::protocol::Transaction
{
public:
    explicit TransactionImpl(std::function<bcostars::Transaction*()> inner)
      : m_inner(std::move(inner))
    {}
    ~TransactionImpl() override = default;

    friend class TransactionFactoryImpl;

    bool operator==(const Transaction& rhs) const { return this->hash() == rhs.hash(); }

    void decode(bcos::bytesConstRef _txData) override;
    void encode(bcos::bytes& txData) const override;

    bcos::crypto::HashType hash() const override;

    void calculateHash(bcos::crypto::hasher::Hasher auto&& hasher)
    {
        bcos::concepts::hash::calculate(
            std::forward<decltype(hasher)>(hasher), *m_inner(), m_inner()->dataHash);
    }

    int32_t version() const override;
    std::string_view chainId() const override;
    std::string_view groupId() const override;
    int64_t blockLimit() const override;
    const std::string& nonce() const override;
    // only for test
    void setNonce(std::string _n) override;
    std::string_view to() const override;
    std::string_view abi() const override;
    bcos::bytesConstRef input() const override;
    int64_t importTime() const override;
    void setImportTime(int64_t _importTime) override;
    bcos::bytesConstRef signatureData() const override;
    std::string_view sender() const override;
    void forceSender(const bcos::bytes& _sender) const override;

    void setSignatureData(bcos::bytes& signature);

    int32_t attribute() const override;
    void setAttribute(int32_t attribute) override;

    std::string_view extraData() const override;
    void setExtraData(std::string const& _extraData) override;

    const bcostars::Transaction& inner() const;
    bcostars::Transaction& mutableInner();
    void setInner(bcostars::Transaction inner);

private:
    std::function<bcostars::Transaction*()> m_inner;
};
}  // namespace bcostars::protocol