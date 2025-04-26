package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;


public class StompServer {

    public static void main(String[] args) {

        if (args.length > 1){
            int port = Integer.parseInt(args[0]);
            String type = args[1]; 

            if (type.equals("tpc")){
                Server.threadPerClient(
                    port, //port
                    () -> new StompMessagingProtocolImpl(), //protocol factory
                    StompEncoderDecoder::new //message encoder decoder factory
                ).serve();                
                
            }

            else { //reactor
                Server.reactor(
                    Runtime.getRuntime().availableProcessors(),
                    port, //port
                    () -> new StompMessagingProtocolImpl(), //protocol factory
                    StompEncoderDecoder::new //message encoder decoder factory
                ).serve();
            }
        }

    }
}
