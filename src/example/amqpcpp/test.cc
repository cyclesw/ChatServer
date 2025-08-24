
int main()
{
    struct ev_loop* loop = EV_DEFAULT;
    AMQP::Address adress("as");
    AMQP::LibEvHandler handler(loop);
    return 0;
}