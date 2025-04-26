package bgu.spl.net.srv;

import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Supplier;

public abstract class BaseServer<T> implements Server<T> {

    private final int port;
    private final Supplier<MessagingProtocol<T>> protocolFactory;
    private final Supplier<MessageEncoderDecoder<T>> encdecFactory;
    private ServerSocket sock;

    private final ConnectionsImpl<T> connections; // Shared ConnectionsImpl instance
    private final AtomicInteger clientIdGenerator; // For generating unique client IDs

    public BaseServer(
            int port,
            Supplier<MessagingProtocol<T>> protocolFactory,
            Supplier<MessageEncoderDecoder<T>> encdecFactory) {

        this.port = port;
        this.protocolFactory = protocolFactory;
        this.encdecFactory = encdecFactory;
        this.sock = null;

        this.connections = new ConnectionsImpl<>(); // Initialize shared connections
        this.clientIdGenerator = new AtomicInteger(0); // Initialize unique client ID generator
    }

    @Override
    public void serve() {
        try (ServerSocket serverSock = new ServerSocket(port)) {
            System.out.println("Server started on port " + port);

            this.sock = serverSock; // To allow proper closing

            while (!Thread.currentThread().isInterrupted()) {
                Socket clientSock = serverSock.accept();

                // Generate a unique client ID
                int clientId = clientIdGenerator.incrementAndGet();

                // Create a new protocol instance and start it with the client ID and connections
                MessagingProtocol<T> protocol = protocolFactory.get();
                protocol.start(clientId, connections);

                // Create a connection handler for the new client
                BlockingConnectionHandler<T> handler = new BlockingConnectionHandler<>(
                        clientSock,
                        encdecFactory.get(),
                        protocol
                );

                // Register the connection handler in the ConnectionsImpl
                connections.addConnection(clientId, handler);

                // Execute the connection handler (e.g., start a new thread for TPC)
                execute(handler);
            }
        } catch (IOException ex) {
            ex.printStackTrace();
        }

        System.out.println("Server closed!!!");
    }

    @Override
    public void close() throws IOException {
        if (sock != null)
            sock.close();
    }

    protected abstract void execute(BlockingConnectionHandler<T> handler);
}
