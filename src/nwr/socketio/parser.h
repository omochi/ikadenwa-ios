//
//  parser.hpp
//  Ikadenwa
//
//  Created by omochimetaru on 2016/02/06.
//  Copyright © 2016年 omochimetaru. All rights reserved.
//

//  JSだと、オブジェクトにバイナリがぶら下がっていても、
//  それを検出してエンコードする機能を持っているが、
//  C++のJson型だとそのバイナリをぶら下げる部分が表現できない
//  socket.io用のオブジェクト型を実装すればインターフェースは保てるが
//  同種の問題はJSコード全般で発生しうるので、
//  Any型を作ったほうがいいかもしれない

#pragma once

#include <string>
#include <vector>

#include <nwr/base/env.h>
#include <nwr/base/data.h>
#include <nwr/base/json.h>
#include <nwr/base/string.h>
#include <nwr/base/emitter.h>

#include "packet.h"

namespace nwr {
namespace sio {
    class BinaryReconstructor;
    
    int parser_protocol();
    
    class Encoder {
    public:
        std::vector<DataPtr> Encode(const Packet & packet);
    };
    
    DataPtr EncodeAsString(const Packet & packet);
    std::vector<DataPtr> EncodeAsBinary(const Packet & packet);
    
    class Decoder {
    public:
        Decoder();
        ~Decoder();
        
        EmitterPtr<Packet> decoded_emitter() { return decoded_emitter_; }
        
        void AddEncodedPacket(const Data & data);
        void AddBinaryData(const Data & data);        
    private:
        std::shared_ptr<BinaryReconstructor> reconstructor_;
        
        EmitterPtr<Packet> decoded_emitter_;
    };
    
    Packet DecodeString(const Data & data);
    
    class BinaryReconstructor {
    public:
        BinaryReconstructor(const Packet & packet);
        
        const Packet & recon_pack() { return recon_pack_; }
        
        Optional<Packet> TakeBinaryData(const Data & data);
        void FinishedReconstruction();
    private:
        Packet recon_pack_;
        std::vector<DataPtr> buffers_;
    };
    
    Packet ParserError();
}
}