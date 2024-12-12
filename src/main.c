static probe *udp_check_reply(int sk, int err, sockaddr_any *from,
                              char *buf, size_t len)
{
    probe *pb;

    pb = probe_by_sk(sk);
    if (!pb)
        return NULL;

    if (pb->seq != from->sin.sin_port)
        return NULL;

    if (!err)
        pb->final = 1;

    return pb;
}



int main(int argc, char *argv[])
{
    
    return 0;
}