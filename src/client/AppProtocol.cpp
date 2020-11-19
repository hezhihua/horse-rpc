/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#include "util/tc_epoll_server.h"

#include "client/AppProtocol.h"
#include "client/Transceiver.h"
#include "client/AdapterProxy.h"

#include "client/protocol/Tars.h"
#include <iostream>


namespace horsedb
{

class Transceiver;

vector<char> ProxyProtocol::tarsRequest(RequestPacket& request, Transceiver *)
{
	TarsOutputStream<BufferWriterVector> os;

	horsedb::Int32 iHeaderLen = 0;

//	先预留4个字节长度
	os.writeBuf((const char *)&iHeaderLen, sizeof(iHeaderLen));

	request.writeTo(os);

    vector<char> buff;

	buff.swap(os.getByteBuffer());

	assert(buff.size() >= 4);

	iHeaderLen = htonl((int)(buff.size()));

	memcpy((void*)buff.data(), (const char *)&iHeaderLen, sizeof(iHeaderLen));

    return buff;
}

////////////////////////////////////////////////////////////////////////////////////

// vector<char> ProxyProtocol::http1Request(tars::RequestPacket& request, Transceiver *trans)
// {
// //	assert(trans->getAdapterProxy()->getObjProxy()->getServantProxy()->taf_connection_serial() > 0);

// //	request.iRequestId = trans->getAdapterProxy()->getId();

// 	shared_ptr<TC_HttpRequest> &data = *(shared_ptr<TC_HttpRequest>*)request.sBuffer.data();

// 	vector<char> buffer;

// 	data->encode(buffer);

// 	data.reset();

// 	return buffer;
// }

// TC_NetWorkBuffer::PACKET_TYPE ProxyProtocol::http1Response(TC_NetWorkBuffer &in, ResponsePacket& rsp)
// {
// 	shared_ptr<TC_HttpResponse> *context = (shared_ptr<TC_HttpResponse>*)(in.getContextData());

//     if(!context)
//     {
//         context = new shared_ptr<TC_HttpResponse>();
//         *context = std::make_shared<TC_HttpResponse>();
//         in.setContextData(context, [=]{ delete context; }); 
//     }

//     if((*context)->incrementDecode(in))
//     {
// 	    rsp.sBuffer.resize(sizeof(shared_ptr<TC_HttpResponse>));

// 	    shared_ptr<TC_HttpResponse> &data = *(shared_ptr<TC_HttpResponse>*)rsp.sBuffer.data();

// 	    data = *context;

// 	    if(!data->checkHeader("Connection", "keep-alive"))
// 	    {
// 		    Transceiver* session = (Transceiver*)(in.getConnection());

// 		    session->close();
// 	    }

// 		(*context) = NULL;
// 		delete context;
// 		in.setContextData(NULL);

// 	    return TC_NetWorkBuffer::PACKET_FULL;

//     }

//     return TC_NetWorkBuffer::PACKET_LESS;
// }

/////////////////////////////////////////////////////////////////////////////////////////////////

// vector<char> ProxyProtocol::httpJceRequest(taf::BasePacket& request, Transceiver *trans)
// {
// 	TC_HttpRequest httpRequest;

// 	string uri;
// 	if(trans->isSSL())
// 		uri = "https://";
// 	else
// 		uri = "http://";

// 	uri += trans->getEndpointInfo().getEndpoint().getHost();

// 	vector<char> buff = tafRequest(request, trans);

// 	for(auto it = request.context.begin(); it != request.context.end(); ++it)
// 	{
// 		if(it->second == ":path")
// 		{
// 			uri += "/" + it->second;
// 		}
// 		else
// 		{
// 			httpRequest.setHeader(it->first, it->second);
// 		}
// 	}

// 	httpRequest.setPostRequest(uri, buff.data(), buff.size(), true);

// 	vector<char> buffer;

// 	httpRequest.encode(buffer);

// 	return buffer;
// }

// TC_NetWorkBuffer::PACKET_TYPE ProxyProtocol::httpJceResponse(TC_NetWorkBuffer &in, BasePacket& rsp)
// {
// 	TC_HttpResponse *context = (TC_HttpResponse*)(in.getContextData());

// 	if(!context)
// 	{
// 		context = new TC_HttpResponse();
// 		in.setContextData(context, [=]{ delete context; });
// 	}

// 	if(context->incrementDecode(in))
// 	{
// 		if(context->getStatus() != 200)
// 		{
// 			rsp.iRet = taf::JCESERVERUNKNOWNERR;
// 			rsp.sResultDesc = context->getContent();
// 			return TC_NetWorkBuffer::PACKET_FULL;
// 		}

// 		JceInputStream<> is;
// 		is.setBuffer(context->getContent().c_str() + 4, context->getContent().size() - 4);

// 		rsp.readFrom(is);

// 		if(!context->checkHeader("Connection", "keep-alive"))
// 		{
// 			Transceiver* session = (Transceiver*)(in.getConnection());

// 			session->close();
// 		}

// 		context = NULL;
// 		delete context;
// 		in.setContextData(NULL);

// 		return TC_NetWorkBuffer::PACKET_FULL;
// 	}

// 	return TC_NetWorkBuffer::PACKET_LESS;
// }


}

