//
//  namespace.cpp
//  Ikadenwa
//
//  Created by omochimetaru on 2016/02/25.
//  Copyright © 2016年 omochimetaru. All rights reserved.
//

#include "namespace.h"

#include "socket.h"

namespace nwr {
namespace sio0 {
    Socket::Socket(const std::shared_ptr<CoreSocket> & socket,
                   const std::string & name):
    emitter_(std::make_shared<AnyEmitter>())
    {
        socket_ = socket;
        name_ = name;
        flags_.clear();
        ack_packets_ = 0;
        acks_.clear();
    }
    
    AnyEmitterPtr Socket::emitter() {
        return emitter_;
    }
    
    std::shared_ptr<Socket> Socket::Of(const std::string & name) {
        return socket_->Of(name);
    }
    
    void Socket::SendPacket(const Packet & arg_packet) {
        Packet packet = arg_packet;
        packet.endpoint = name_;
        socket_->SendPacket(packet);
        flags_.clear();
    }
    
    void Socket::Send(const Any & data, const std::function<void(const Any &)> & fn) {
        Packet packet;
        packet.type = flags_["json"] ? PacketType::Json : PacketType::Message;
        packet.data = data;
        
        if (fn) {
            ack_packets_ += 1;
            packet.id = Some(ack_packets_);
            packet.ack = Some(std::string());
            acks_[*packet.id] = fn;
        }
        
        SendPacket(packet);
    }
    
    void Socket::Emit(const std::string & name,
                      const Any & args,
                      const std::function<void(const Any &)> & ack)
    {
        Packet packet;
        packet.type = PacketType::Event;
        packet.name = name;
        
        if (ack) {
            ack_packets_ += 1;
            packet.id = Some(ack_packets_);
            packet.ack = Some(std::string("data"));
            acks_[*packet.id] = ack;
        }
        packet.args = args;
        
        SendPacket(packet);
    }
    
    void Socket::Disconnect() {
        if (name_ == "") {
            socket_->Disconnect();
        } else {
            Packet packet;
            packet.type = PacketType::Disconnect;
            SendPacket(packet);
            emitter_->Emit("disconnect", {});
        }
    }
    
    void Socket::OnPacket(const Packet & packet) {
        auto thiz = shared_from_this();
        
        auto ack = [thiz, packet](const Any & args){
            Packet pkt;
            pkt.type = PacketType::Ack;
            pkt.args = args;
            pkt.ack_id = packet.id.value();
            thiz->SendPacket(pkt);
        };
        
        switch (packet.type) {
            case PacketType::Connect: {
                emitter_->Emit("connect", {});
                break;
            }
            case PacketType::Disconnect: {
                if (name_ == "") {
                    socket_->OnDisconnect(packet.reason || std::string("booted"));
                } else {
                    emitter_->Emit("disconnect",
                                   { Any(packet.reason) }
                                   );
                }
                break;
            }
            case PacketType::Message:
            case PacketType::Json: {
                std::vector<Any> params = { packet.data };
                
                if (packet.ack == Some(std::string("data")) ){
                    params.push_back(AnyFuncMake(ack));
                } else if (packet.ack) {
                    Packet pkt;
                    pkt.type = PacketType::Ack;
                    pkt.ack_id = *packet.id;
                    SendPacket(pkt);
                }
                
                emitter_->Emit("message", params);
                
                break;
            }
            case PacketType::Event: {
                std::vector<Any> params = packet.args.AsArray().value();
                
                if (packet.ack == Some(std::string("data"))) {
                    params.push_back(AnyFuncMake(ack));
                }
                
                emitter_->Emit(packet.name, params);
                break;
            }
            case PacketType::Ack: {
                if (HasKey(acks_, packet.ack_id)) {
                    acks_[packet.ack_id](packet.args);
                    acks_.erase(packet.ack_id);
                }
                break;
            }
            case PacketType::Error: {
                if (packet.advice) {
                    socket_->OnError(packet.reason || std::string(), packet.advice || std::string());
                } else {
                    if (packet.reason == Some(std::string("unauthorized")) ){
                        emitter_->Emit("connect_failed", { Any(packet.reason) });
                    } else {
                        emitter_->Emit("error", { Any(packet.reason) });
                    }
                }
                break;
            }
            default: break;
        }
    }
}
}

