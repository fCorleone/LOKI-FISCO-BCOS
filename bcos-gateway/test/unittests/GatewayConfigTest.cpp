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
 * @brief test for gateway
 * @file GatewayConfigTest.cpp
 * @author: octopus
 * @date 2021-05-17
 */

#include <bcos-gateway/GatewayConfig.h>
#include <bcos-gateway/GatewayFactory.h>
#include <bcos-utilities/testutils/TestPromptFixture.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

using namespace bcos;
using namespace gateway;
using namespace bcos::test;

BOOST_FIXTURE_TEST_SUITE(GatewayConfigTest, TestPromptFixture)

BOOST_AUTO_TEST_CASE(test_validPort)
{
    auto config = std::make_shared<GatewayConfig>();
    BOOST_CHECK(!config->isValidPort(1024));
    BOOST_CHECK(!config->isValidPort(65536));
    BOOST_CHECK(config->isValidPort(30300));
}

BOOST_AUTO_TEST_CASE(test_validIP)
{
    auto config = std::make_shared<GatewayConfig>();

    BOOST_CHECK(!config->isValidIP("a"));
    BOOST_CHECK(!config->isValidIP("127"));
    BOOST_CHECK(!config->isValidIP("127.0"));
    BOOST_CHECK(!config->isValidIP("127.0.0"));
    BOOST_CHECK(!config->isValidIP("127.0.0.1.0"));

    // ipv4
    BOOST_CHECK(config->isValidIP("127.0.0.1"));
    BOOST_CHECK(config->isValidIP("192.168.0.1"));
    BOOST_CHECK(config->isValidIP("64.120.121.206"));

    // ipv6
    BOOST_CHECK(config->isValidIP("::1"));
    BOOST_CHECK(config->isValidIP("fe80::58da:28ff:fe08:5d91"));
    BOOST_CHECK(config->isValidIP("1111::1111:1111:1111:1111"));
}

BOOST_AUTO_TEST_CASE(test_hostAndPort2Endpoint)
{
    auto config = std::make_shared<GatewayConfig>();

    {
        NodeIPEndpoint endpoint;
        BOOST_CHECK_NO_THROW(config->hostAndPort2Endpoint("127.0.0.1:1111", endpoint));
        BOOST_CHECK_EQUAL(endpoint.address(), "127.0.0.1");
        BOOST_CHECK_EQUAL(endpoint.port(), 1111);
        BOOST_CHECK(!endpoint.isIPv6());
    }

    {
        NodeIPEndpoint endpoint;
        BOOST_CHECK_NO_THROW(config->hostAndPort2Endpoint("[::1]:1234", endpoint));
        BOOST_CHECK_EQUAL(endpoint.address(), "::1");
        BOOST_CHECK_EQUAL(endpoint.port(), 1234);
        BOOST_CHECK(endpoint.isIPv6());
    }

    {
        NodeIPEndpoint endpoint;
        BOOST_CHECK_NO_THROW(config->hostAndPort2Endpoint("8.129.188.218:12345", endpoint));
        BOOST_CHECK_EQUAL(endpoint.address(), "8.129.188.218");
        BOOST_CHECK_EQUAL(endpoint.port(), 12345);
        BOOST_CHECK(!endpoint.isIPv6());
    }

    {
        NodeIPEndpoint endpoint;
        BOOST_CHECK_NO_THROW(
            config->hostAndPort2Endpoint("[fe80::1a9d:50ae:3207:80d9]:54321", endpoint));
        BOOST_CHECK_EQUAL(endpoint.address(), "fe80::1a9d:50ae:3207:80d9");
        BOOST_CHECK_EQUAL(endpoint.port(), 54321);
        BOOST_CHECK(endpoint.isIPv6());
    }

    {
        NodeIPEndpoint endpoint;
        BOOST_CHECK_THROW(config->hostAndPort2Endpoint("abcdef:fff", endpoint), std::exception);
        BOOST_CHECK_THROW(config->hostAndPort2Endpoint("127.0.0.1", endpoint), std::exception);
    }
}

BOOST_AUTO_TEST_CASE(test_nodesJsonParser)
{
    {
        std::string json =
            "{\"nodes\":[\"127.0.0.1:30300\",\"127.0.0.1:30301\","
            "\"127.0.0.1:30302\"]}";
        auto config = std::make_shared<GatewayConfig>();
        std::set<NodeIPEndpoint> nodeIPEndpointSet;
        config->parseConnectedJson(json, nodeIPEndpointSet);
        BOOST_CHECK_EQUAL(nodeIPEndpointSet.size(), 3);
        BOOST_CHECK_EQUAL(config->threadPoolSize(), 8);
    }

    {
        std::string json = "{\"nodes\":[]}";
        auto config = std::make_shared<GatewayConfig>();
        std::set<NodeIPEndpoint> nodeIPEndpointSet;
        config->parseConnectedJson(json, nodeIPEndpointSet);
        BOOST_CHECK_EQUAL(nodeIPEndpointSet.size(), 0);
        BOOST_CHECK_EQUAL(config->threadPoolSize(), 8);
    }

    {
        std::string json =
            "{\"nodes\":[\"["
            "fe80::1a9d:50ae:3207:80d9]:30302\","
            "\"[fe80::1a9d:50ae:3207:80d9]:30303\"]}";
        auto config = std::make_shared<GatewayConfig>();
        std::set<NodeIPEndpoint> nodeIPEndpointSet;
        config->parseConnectedJson(json, nodeIPEndpointSet);
        BOOST_CHECK_EQUAL(nodeIPEndpointSet.size(), 2);
        BOOST_CHECK_EQUAL(config->threadPoolSize(), 8);
    }
}

BOOST_AUTO_TEST_CASE(test_initConfig)
{
    {
        // std::string
        // configIni("../../../bcos-gateway/test/unittests/data/config/config_ipv4.ini");
        std::string configIni("data/config/config_ipv4.ini");
        auto config = std::make_shared<GatewayConfig>();
        config->initConfig(configIni);
        config->loadP2pConnectedNodes();

        BOOST_CHECK_EQUAL(config->listenIP(), "127.0.0.1");
        BOOST_CHECK_EQUAL(config->listenPort(), 12345);
        BOOST_CHECK_EQUAL(config->smSSL(), false);
        BOOST_CHECK_EQUAL(config->connectedNodes().size(), 3);

        auto certConfig = config->certConfig();
        BOOST_CHECK(!certConfig.caCert.empty());
        BOOST_CHECK(!certConfig.nodeCert.empty());
        BOOST_CHECK(!certConfig.nodeKey.empty());
    }
}

BOOST_AUTO_TEST_CASE(test_initSMConfig)
{
    {
        // std::string
        // configIni("../../../bcos-gateway/test/unittests/data/config/config_ipv6.ini");
        std::string configIni("data/config/config_ipv6.ini");

        auto config = std::make_shared<GatewayConfig>();
        config->initConfig(configIni);
        config->loadP2pConnectedNodes();

        BOOST_CHECK_EQUAL(config->listenIP(), "0.0.0.0");
        BOOST_CHECK_EQUAL(config->listenPort(), 54321);
        BOOST_CHECK_EQUAL(config->smSSL(), true);
        BOOST_CHECK_EQUAL(config->connectedNodes().size(), 1);

        auto smCertConfig = config->smCertConfig();
        BOOST_CHECK(!smCertConfig.caCert.empty());
        BOOST_CHECK(!smCertConfig.nodeCert.empty());
        BOOST_CHECK(!smCertConfig.nodeKey.empty());
        BOOST_CHECK(!smCertConfig.enNodeCert.empty());
        BOOST_CHECK(!smCertConfig.enNodeKey.empty());
    }
}

BOOST_AUTO_TEST_CASE(test_initFlowControlConfig)
{
    {
        bcos::gateway::GatewayConfig::RateLimiterConfig rateLimiterConfig;
        BOOST_CHECK_EQUAL(rateLimiterConfig.timeWindowSec, 1);
        BOOST_CHECK(!rateLimiterConfig.allowExceedMaxPermitSize);
        BOOST_CHECK(!rateLimiterConfig.enableDistributedRatelimit);
        BOOST_CHECK(!rateLimiterConfig.enableOutRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableInRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableOutConnRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableOutGroupRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableInP2pBasicMsgLimit());
        BOOST_CHECK(!rateLimiterConfig.enableInP2pModuleMsgLimit(1));
    }

    {
        auto config = std::make_shared<GatewayConfig>();
        boost::property_tree::ptree pt;
        config->initFlowControlConfig(pt);
        auto rateLimiterConfig = config->rateLimiterConfig();

        BOOST_CHECK_EQUAL(rateLimiterConfig.timeWindowSec, 1);
        BOOST_CHECK(!rateLimiterConfig.allowExceedMaxPermitSize);
        BOOST_CHECK(!rateLimiterConfig.enableDistributedRatelimit);
        BOOST_CHECK(!rateLimiterConfig.enableOutRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableInRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableOutConnRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableOutGroupRateLimit());
        BOOST_CHECK(!rateLimiterConfig.enableInP2pBasicMsgLimit());
        BOOST_CHECK(!rateLimiterConfig.enableInP2pModuleMsgLimit(1));
    }

    {
        std::string configIni("data/config/config_ipv4.ini");

        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(configIni, pt);

        auto config = std::make_shared<GatewayConfig>();
        config->initFlowControlConfig(pt);

        auto rateLimiterConfig = config->rateLimiterConfig();
        auto timeWindowSec = rateLimiterConfig.timeWindowSec;

        BOOST_CHECK_EQUAL(rateLimiterConfig.timeWindowSec, 3);
        BOOST_CHECK(rateLimiterConfig.enableDistributedRatelimit);
        BOOST_CHECK(rateLimiterConfig.enableDistributedRateLimitCache);
        BOOST_CHECK_EQUAL(rateLimiterConfig.distributedRateLimitCachePercent, 13);
        BOOST_CHECK_EQUAL(rateLimiterConfig.statInterval, 12345);

        BOOST_CHECK(rateLimiterConfig.enableOutRateLimit());
        BOOST_CHECK(rateLimiterConfig.enableInRateLimit());
        BOOST_CHECK(rateLimiterConfig.enableInP2pBasicMsgLimit());
        BOOST_CHECK(rateLimiterConfig.enableOutConnRateLimit());
        BOOST_CHECK(rateLimiterConfig.enableOutGroupRateLimit());
        BOOST_CHECK(rateLimiterConfig.allowExceedMaxPermitSize);

        BOOST_CHECK_EQUAL(rateLimiterConfig.totalOutgoingBwLimit, config->doubleMBToBit(10));
        BOOST_CHECK_EQUAL(rateLimiterConfig.connOutgoingBwLimit, config->doubleMBToBit(2));
        BOOST_CHECK_EQUAL(rateLimiterConfig.groupOutgoingBwLimit, config->doubleMBToBit(5));

        BOOST_CHECK_EQUAL(rateLimiterConfig.modulesWithoutLimit.size(), 6);
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::Raft) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::PBFT) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::TxsSync) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::AMOP) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(5001) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(5002) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(5003) ==
                    rateLimiterConfig.modulesWithoutLimit.end());

        BOOST_CHECK_EQUAL(rateLimiterConfig.ip2BwLimit.size(), 3);
        BOOST_CHECK_EQUAL(
            rateLimiterConfig.ip2BwLimit.find("192.108.0.1")->second, config->doubleMBToBit(1));
        BOOST_CHECK_EQUAL(
            rateLimiterConfig.ip2BwLimit.find("192.108.0.2")->second, config->doubleMBToBit(2));
        BOOST_CHECK_EQUAL(
            rateLimiterConfig.ip2BwLimit.find("192.108.0.3")->second, config->doubleMBToBit(3));

        BOOST_CHECK_EQUAL(rateLimiterConfig.group2BwLimit.size(), 3);
        BOOST_CHECK_EQUAL(
            rateLimiterConfig.group2BwLimit.find("group0")->second, config->doubleMBToBit(2));
        BOOST_CHECK_EQUAL(
            rateLimiterConfig.group2BwLimit.find("group1")->second, config->doubleMBToBit(2));
        BOOST_CHECK_EQUAL(
            rateLimiterConfig.group2BwLimit.find("group2")->second, config->doubleMBToBit(2));

        BOOST_CHECK(rateLimiterConfig.enableInP2pModuleMsgLimit(1));
        BOOST_CHECK(rateLimiterConfig.enableInP2pModuleMsgLimit(5));
        BOOST_CHECK(rateLimiterConfig.enableInP2pModuleMsgLimit(7));
        BOOST_CHECK(rateLimiterConfig.enableInP2pModuleMsgLimit(8));

        BOOST_CHECK_EQUAL(rateLimiterConfig.p2pBasicMsgTypes.size(), 5);  // 1,2,3,6,9
        BOOST_CHECK(rateLimiterConfig.p2pBasicMsgTypes.contains(1));
        BOOST_CHECK(!rateLimiterConfig.p2pBasicMsgTypes.contains(4));
        BOOST_CHECK_EQUAL(rateLimiterConfig.p2pBasicMsgQPS, 123);
        BOOST_CHECK_EQUAL(rateLimiterConfig.p2pModuleMsgQPS, 555);
        BOOST_CHECK_EQUAL(rateLimiterConfig.moduleMsg2QPSSize, 3);
        BOOST_CHECK_EQUAL(rateLimiterConfig.moduleMsg2QPS.size(), uint16_t(-1));
        BOOST_CHECK_EQUAL(rateLimiterConfig.moduleMsg2QPS[1], 123);
        BOOST_CHECK_EQUAL(rateLimiterConfig.moduleMsg2QPS[5], 456);
        BOOST_CHECK_EQUAL(rateLimiterConfig.moduleMsg2QPS[7], 789);
    }

    {
        std::string configIni("data/config/config_ipv6.ini");

        boost::property_tree::ptree pt;
        boost::property_tree::ini_parser::read_ini(configIni, pt);

        auto config = std::make_shared<GatewayConfig>();
        config->initFlowControlConfig(pt);

        auto rateLimiterConfig = config->rateLimiterConfig();

        BOOST_CHECK(rateLimiterConfig.enableOutRateLimit());

        BOOST_CHECK(!rateLimiterConfig.enableDistributedRatelimit);
        BOOST_CHECK(rateLimiterConfig.enableDistributedRateLimitCache);
        BOOST_CHECK_EQUAL(rateLimiterConfig.distributedRateLimitCachePercent, 20);
        BOOST_CHECK_EQUAL(rateLimiterConfig.statInterval, 60000);

        BOOST_CHECK(rateLimiterConfig.enableOutRateLimit());
        BOOST_CHECK(rateLimiterConfig.enableOutGroupRateLimit());
        BOOST_CHECK(rateLimiterConfig.enableOutGroupRateLimit());

        BOOST_CHECK_EQUAL(rateLimiterConfig.totalOutgoingBwLimit, config->doubleMBToBit(3));
        BOOST_CHECK_EQUAL(rateLimiterConfig.connOutgoingBwLimit, config->doubleMBToBit(2));
        BOOST_CHECK_EQUAL(rateLimiterConfig.groupOutgoingBwLimit, config->doubleMBToBit(1));

        BOOST_CHECK_EQUAL(rateLimiterConfig.modulesWithoutLimit.size(), 4);
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::Raft) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::PBFT) !=
                    rateLimiterConfig.modulesWithoutLimit.end());
        BOOST_CHECK(
            rateLimiterConfig.modulesWithoutLimit.find(bcos::protocol::ModuleID::ConsTxsSync) !=
            rateLimiterConfig.modulesWithoutLimit.end());

        BOOST_CHECK_EQUAL(rateLimiterConfig.ip2BwLimit.size(), 0);
        BOOST_CHECK_EQUAL(rateLimiterConfig.group2BwLimit.size(), 0);
    }
}

BOOST_AUTO_TEST_CASE(test_doubleMBToBit)
{
    auto config = std::make_shared<GatewayConfig>();
    BOOST_CHECK_EQUAL(config->doubleMBToBit(1.0), 1024 * 1024 / 8);

    BOOST_CHECK_EQUAL(config->doubleMBToBit(2.5), 25 * 1024 * 1024 / 8 / 10);

    // BOOST_CHECK_EQUAL(config->doubleMBToBit(10.0), 10 * 1024 * 1024 / 8 / 10);

    BOOST_CHECK_EQUAL(config->doubleMBToBit(25.5), 255 * 1024 * 1024 / 8 / 10);

    BOOST_CHECK_EQUAL(config->doubleMBToBit(100), 100 * 1024 * 1024 / 8);
}

BOOST_AUTO_TEST_CASE(test_RedisConfig)
{
    auto config = std::make_shared<GatewayConfig>();
    std::string configIni("data/config/config_ipv6.ini");

    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(configIni, pt);

    config->initRedisConfig(pt);

    BOOST_CHECK_EQUAL(config->redisConfig().host, "127.127.127.127");
    BOOST_CHECK_EQUAL(config->redisConfig().port, 12345);
    BOOST_CHECK_EQUAL(config->redisConfig().connectionPoolSize, 111);
    BOOST_CHECK_EQUAL(config->redisConfig().timeout, 54321);
    BOOST_CHECK_EQUAL(config->redisConfig().password, "abc");
    BOOST_CHECK_EQUAL(config->redisConfig().db, 12);
}

BOOST_AUTO_TEST_SUITE_END()
