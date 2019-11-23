//
// Created by Олег Бобров on 19.11.2019.
//

#include <fstream>
#include <iostream>
#include "GraphController.h"
#include "../static/Database.h"
#include "Post.h"
#include "../static/PrefabCreator.h"
#include "../Networking/Connection.hpp"
#include "../Networking/PacketQueue.hpp"
#include "../Networking/Packet.hpp"


void GraphController::applyLayer10(const nlohmann::json &json) {
    for (const auto & item : json["coordinates"]) {
        int idx = -1;
        if (!item.tryGetValue("idx", idx)) {
            continue;
        }
        auto & point = Database::points[idx];
        if (!point) {
            point = PrefabCreator::createPoint(idx);
            point->transform->setParent(GraphController::transform);//TODO: change to Instantiate(original, parent)
        }
        point->applyLayer10(item);
    }
}

void GraphController::applyLayer1(const nlohmann::json &json) {
    for (const auto & item : json["posts"]) {
        int idx = -1;
        if (!item.tryGetValue("idx", idx)) {
            continue;
        }
        auto & post = Database::posts[idx];
        PostType type = PostType::DEFAULT;
        item.tryGetValue("type", type);
        switch (type) {
            case PostType::TOWN: {
                if (!post) {
                    post = PrefabCreator::createTown(idx);
                }
                post->applyLayer1(item);
                break;
            }
            case MARKET: {
                if (!post) {
                    post = PrefabCreator::createMarket(idx);
                }
                post->applyLayer1(item);
                break;
            }
            case STORAGE: {
                if (!post) {
                    post = PrefabCreator::createStorage(idx);
                }
                post->applyLayer1(item);
                break;
            }
            default: {
                break;
            }
        }
    }
    for (const auto & item : json["trains"]) {
        int idx = -1;
        if (!item.tryGetValue("idx", idx)) {
            continue;
        }
        auto &train = Database::trains[idx];
        if (!train) {
            train = PrefabCreator::createTrain(idx);
        }
        train->applyLayer1(item);
    }
}

void GraphController::applyLayer0(const nlohmann::json &json) {
    for (const auto & item : json["points"]) {
        int idx = -1;
        if (!item.tryGetValue("idx", idx)) {
            continue;
        }
        auto& point = Database::points[idx];
        if (!point) {
            point = PrefabCreator::createPoint(idx);
            point->transform->setParent(GraphController::transform);
            //TODO: change to Instantiate(original, parent)
        }
        point->applyLayer0(item);
    }
    for (const auto & item : json["lines"]) {
        int idx = -1;
        item.tryGetValue("idx", idx);
        auto & line = Database::lines[idx];
        if (!line) {
            line = PrefabCreator::createLine(idx);
            line->transform->setParent(GraphController::transform);//TODO: change to Instantiate(original, parent)
        }
        line->applyLayer0(item);
        //for graph
        nlohmann::json item_points;
        item.tryGetValue("points", item_points);
        graph[item_points[0]].push_back(item_points[1]);
        graph[item_points[1]].push_back(item_points[0]);
    }
}

void GraphController::applyForceMethod() {
    auto positions = graphVisualizer.forceMethod();
    sf::Vector2f center = {0, 0};
    for (auto & pair : Database::points) {
        pair.second->transform->setLocalPosition(positions[pair.second->idx]);
        center += positions[pair.second->idx];
    }
    center /= static_cast<float>(Database::points.size());
    GraphController::transform->setPosition(-center);
    //Camera::mainCamera->transform->setLocalPosition(center);
}

void GraphController::applyForceMethodIteration() {
    auto positions = graphVisualizer.forceMethodIteration();
    sf::Vector2f center = {0, 0};
    for (auto & pair : Database::points) {
        pair.second->transform->setLocalPosition(positions[pair.second->idx]);
        center += positions[pair.second->idx];
    }
    center /= static_cast<float>(Database::points.size());
    GraphController::transform->setPosition(-center);
    //Camera::mainCamera->transform->setLocalPosition(center);
}

void checkResult(const Packet & received) {
    if (received.getFlag() != Result::OKEY) {
        std::cout << "something went wrong. Returned code: " << 
                     received.getFlag() << std::endl;
        std::cout << received.getJson();
        exit(1);
    }
}

void GraphController::update() {
    if (!called) {
        //TODO: do all network stuff here
        called = true;
        
        Connection::login();
        
        PacketQueue & pQueue = PacketQueue::instance();
        json message;
        message["name"] = "Pasha";
        pQueue.sendPacket(Packet(Action::LOGIN, message));
        
        
        pQueue.wait();
        
        std::pair<Packet, int32_t> received = pQueue.receivePacket();
        
        checkResult(received.first);        
        
        playerInfo = received.first.getJson();
        
        nlohmann::json layer0, layer1;
        
        message.clear();
        
        message["layer"] = 0;
        pQueue.sendPacket(Packet(Action::MAP, message));
        
        message.clear();
        message["layer"] = 1;
        pQueue.sendPacket(Packet(Action::MAP, message));
        
        pQueue.wait();
        
        received = pQueue.receivePacket();
        checkResult(received.first);
        layer0 = received.first.getJson();
        
        received = pQueue.receivePacket();
        checkResult(received.first);
        layer1 = received.first.getJson();
                
        GraphController::applyLayer0(layer0);
        GraphController::applyLayer1(layer1);
        GraphController::graphVisualizer.setGraph(GraphController::graph);
    }
    
    // TODO normal timer for iterations
    for (int i = 0; i < 40; i++) {
        GraphController::applyForceMethodIteration();
    }
}
