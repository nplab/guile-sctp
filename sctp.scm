(define-module (net sctp))

(export sctp-recvmsg!
        sctp-sendmsg

        IPPROTO_SCTP
        MSG_NOTIFICATION
        MSG_EOR
        SCTP_NODELAY
        SCTP_UNORDERED
        SCTP_ADDR_OVER
        SCTP_ABORT
        SCTP_EOF
        SCTP_EOR
        SCTP_PR_SCTP_TTL
        SCTP_PR_SCTP_BUF
        SCTP_PR_SCTP_RTX)

(load-extension "libguile-net-sctp" "net_sctp_init")
