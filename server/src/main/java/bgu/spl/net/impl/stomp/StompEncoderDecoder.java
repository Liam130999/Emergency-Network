package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.MessageEncoderDecoder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

public class StompEncoderDecoder implements MessageEncoderDecoder<String> {

    private byte[] bytes = new byte[1 << 10]; // Start with 1k buffer
    private int len = 0;

    @Override
    public String decodeNextByte(byte nextByte) {
        // The null character (\u0000) marks the end of a STOMP frame
        if (nextByte == '\u0000') {
            return popString();
        }

        pushByte(nextByte);
        return null; // Frame is not complete yet
    }

    @Override
    public byte[] encode(String message) {
        // Append the null character to the end of the message and convert to bytes
        //System.out.println(message);
        return (message + "\0").getBytes(StandardCharsets.UTF_8);
    }

    private void pushByte(byte nextByte) {
        // Ensure enough space in the buffer
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2); // Double the size of the buffer
        }

        bytes[len++] = nextByte;
    }

    private String popString() {
        // Convert the accumulated bytes to a UTF-8 string
        String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
        len = 0; // Reset the buffer
        return result;
    }
}
