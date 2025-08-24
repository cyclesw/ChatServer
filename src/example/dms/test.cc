#include "dms.h"
#include <string>

int main()
{
    using namespace im;
    std::string key("gL1QGmWRK08lRD65");
    std::string phone = "18475266821";
    std::string code = "1234";
    DMSClient client(key);

    client.Send(phone, code);

    return 0;
}