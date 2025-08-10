//
// Created by 19396 on 25-8-1.
//

#ifndef ASR_H
#define ASR_H

#include "log.hpp"

#include <aip/speech.h>

namespace im
{

class ASRClient
{
public:
    using Ptr = std::shared_ptr<ASRClient>;

    ASRClient(const std::string& app_id,
            const std::string& api_key,
            const std::string& secret_key)
                :_client(app_id, api_key, secret_key)
    {}

    bool Recognize(const std::string& speech_data, std::string& result)
    {
        Json::Value json = _client.recognize(speech_data, "pcm", 16000, aip::null);
        if (json["err_no"].asInt() != 0)
        {
            result = json["err_msg"].asString();
            LOG_ERROR("语音识别失败：{}", result);
            return false;
        }

        result = json["result"][0].asString();
        return true;
    }
private:
    aip::Speech _client;
};

}

#endif //ASR_H
