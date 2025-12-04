package com.themisdb.client;

import java.io.*;
import java.net.*;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.*;
import java.util.zip.CRC32;
import com.google.gson.*;

/**
 * ThemisDB Native Java Client
 * Binary Wire Protocol v1 implementation
 * Performance: 5-10x faster than HTTP REST
 */
public class ThemisDBClient {
    // Constants
    private static final long WIRE_MAGIC = 0x544D4442L; // "TMDB"
    private static final byte WIRE_VERSION = 0x01;
    private static final int HEADER_SIZE = 12;
    private static final int CHECKSUM_SIZE = 4;
    private static final int MAX_PAYLOAD_SIZE = 64 * 1024 * 1024; // 64MB
    private static final int DEFAULT_TIMEOUT = 30000;

    // Operation Codes
    public static final class OpCode {
        public static final byte HELLO = 0x01;
        public static final byte HELLO_ACK = 0x02;
        public static final byte AUTH_REQUEST = 0x03;
        public static final byte AUTH_RESPONSE = 0x04;
        public static final byte AUTH_SUCCESS = 0x05;
        public static final byte AUTH_FAILURE = 0x06;

        public static final byte GET = 0x10;
        public static final byte PUT = 0x11;
        public static final byte DELETE = 0x12;
        public static final byte BATCH_GET = 0x13;
        public static final byte BATCH_PUT = 0x14;

        public static final byte QUERY_AQL = 0x20;
        public static final byte QUERY_RESULT = 0x21;
        public static final byte QUERY_CURSOR = 0x22;
        public static final byte CURSOR_NEXT = 0x23;
        public static final byte CURSOR_CLOSE = 0x24;

        public static final byte TRANSACTION_BEGIN = 0x30;
        public static final byte TRANSACTION_COMMIT = 0x31;
        public static final byte TRANSACTION_ABORT = 0x32;

        public static final byte VECTOR_SEARCH = 0x40;
        public static final byte GRAPH_TRAVERSE = 0x41;

        public static final byte GEO_QUERY = 0x50;
        public static final byte TIMESERIES_QUERY = 0x51;

        public static final byte BPMN_START_PROCESS = 0x60;
        public static final byte BPMN_TASK_COMPLETE = 0x61;
        public static final byte BPMN_QUERY_INSTANCE = 0x62;

        public static final byte ERROR = (byte) 0xF0;
        public static final byte OK = (byte) 0xF1;
        public static final byte PING = (byte) 0xFE;
        public static final byte CLOSE = (byte) 0xFF;
    }

    // Message Flags
    public static final class MessageFlags {
        public static final int NONE = 0x0000;
        public static final int SKIP_CHECKSUM = 0x0001;
        public static final int COMPRESSED = 0x0002;
        public static final int ENCRYPTED = 0x0004;
    }

    // Exceptions
    public static class ThemisDBException extends Exception {
        public ThemisDBException(String message) {
            super(message);
        }

        public ThemisDBException(String message, Throwable cause) {
            super(message, cause);
        }
    }

    public static class AuthenticationException extends ThemisDBException {
        public AuthenticationException(String message) {
            super(message);
        }
    }

    public static class ConnectionException extends ThemisDBException {
        public ConnectionException(String message) {
            super(message);
        }
    }

    // Wire Frame
    public static class WireFrame {
        public byte version;
        public byte opcode;
        public short flags;
        public int payloadSize;
        public int sequence;
        public byte[] payload;

        public WireFrame(byte opcode, byte[] payload, short flags, int sequence) {
            this.version = WIRE_VERSION;
            this.opcode = opcode;
            this.flags = flags;
            this.payload = payload != null ? payload : new byte[0];
            this.payloadSize = this.payload.length;
            this.sequence = sequence;
        }

        public byte[] toBytes() throws IOException {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(baos);

            // Header
            dos.writeInt((int) WIRE_MAGIC);
            dos.writeByte(version);
            dos.writeByte(opcode);
            dos.writeShort(flags);
            dos.writeInt(payloadSize);

            // Payload
            dos.write(payload);

            // Checksum
            if ((flags & MessageFlags.SKIP_CHECKSUM) == 0) {
                byte[] headerAndPayload = baos.toByteArray();
                long checksum = calculateCRC32(headerAndPayload);
                dos.writeInt((int) checksum);
            }

            return baos.toByteArray();
        }

        public static WireFrame fromBytes(byte[] data, int offset) throws IOException {
            DataInputStream dis = new DataInputStream(new ByteArrayInputStream(data, offset, data.length - offset));

            // Read header
            long magic = dis.readInt() & 0xFFFFFFFFL;
            if (magic != WIRE_MAGIC) {
                throw new IOException("Invalid magic: 0x" + Long.toHexString(magic));
            }

            byte version = dis.readByte();
            byte opcode = dis.readByte();
            short flags = dis.readShort();
            int payloadSize = dis.readInt();

            if (payloadSize > MAX_PAYLOAD_SIZE) {
                throw new IOException("Payload too large: " + payloadSize);
            }

            // Read payload
            byte[] payload = new byte[payloadSize];
            dis.readFully(payload);

            // Read and verify checksum
            if ((flags & MessageFlags.SKIP_CHECKSUM) == 0) {
                int checksum = dis.readInt();
                int headerSize = 12;
                byte[] headerAndPayload = new byte[headerSize + payloadSize];
                System.arraycopy(data, offset, headerAndPayload, 0, headerSize + payloadSize);
                long calculatedChecksum = calculateCRC32(headerAndPayload);
                if ((int) calculatedChecksum != checksum) {
                    throw new IOException("Checksum mismatch");
                }
            }

            WireFrame frame = new WireFrame(opcode, payload, flags, 0);
            frame.version = version;
            return frame;
        }
    }

    // Client
    private Socket socket;
    private InputStream input;
    private OutputStream output;
    private String host;
    private int port;
    private String username;
    private String password;
    private boolean authenticated;
    private int sequence;
    private Map<Integer, CompletableFuture<WireFrame>> pendingRequests;
    private Gson gson;
    private ExecutorService executor;
    private volatile boolean running;

    public ThemisDBClient(String host, int port, String username, String password) {
        this.host = host;
        this.port = port;
        this.username = username;
        this.password = password;
        this.authenticated = false;
        this.sequence = 0;
        this.pendingRequests = new ConcurrentHashMap<>();
        this.gson = new Gson();
        this.executor = Executors.newFixedThreadPool(2);
        this.running = false;
    }

    /**
     * Connect to ThemisDB server
     */
    public void connect() throws ThemisDBException {
        try {
            socket = new Socket(host, port);
            input = socket.getInputStream();
            output = socket.getOutputStream();
            running = true;

            // Start receive thread
            executor.submit(this::receiveLoop);

            authenticate();
        } catch (IOException e) {
            throw new ConnectionException("Failed to connect: " + e.getMessage());
        }
    }

    /**
     * Disconnect from server
     */
    public void disconnect() {
        running = false;
        try {
            if (socket != null) socket.close();
            executor.shutdown();
        } catch (IOException e) {
            // Ignore
        }
    }

    /**
     * Authenticate with server
     */
    private void authenticate() throws ThemisDBException {
        try {
            // Send HELLO
            WireFrame hello = new WireFrame(OpCode.HELLO, "ThemisDB/1.0".getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, 0);
            sendFrame(hello);

            // Send AUTH_REQUEST
            String credentials = username + ":" + password;
            WireFrame auth = new WireFrame(OpCode.AUTH_REQUEST, credentials.getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, 0);
            sendFrame(auth);

            authenticated = true;
        } catch (IOException e) {
            throw new AuthenticationException("Authentication failed: " + e.getMessage());
        }
    }

    /**
     * Get document by key
     */
    public JsonObject get(String key) throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("key", key);
            WireFrame frame = new WireFrame(OpCode.GET, payload.toString().getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, ++sequence);
            WireFrame response = sendAndWait(frame);
            return JsonParser.parseString(new String(response.payload, StandardCharsets.UTF_8))
                    .getAsJsonObject();
        } catch (IOException e) {
            throw new ThemisDBException("Get failed: " + e.getMessage());
        }
    }

    /**
     * Put document
     */
    public void put(String key, JsonObject value) throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("key", key);
            payload.add("value", value);
            WireFrame frame = new WireFrame(OpCode.PUT, payload.toString().getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, ++sequence);
            sendAndWait(frame);
        } catch (IOException e) {
            throw new ThemisDBException("Put failed: " + e.getMessage());
        }
    }

    /**
     * Delete document
     */
    public void delete(String key) throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("key", key);
            WireFrame frame = new WireFrame(OpCode.DELETE, payload.toString().getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, ++sequence);
            sendAndWait(frame);
        } catch (IOException e) {
            throw new ThemisDBException("Delete failed: " + e.getMessage());
        }
    }

    /**
     * Execute AQL query
     */
    public JsonArray query(String aql, Map<String, Object> options) throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("aql", aql);
            if (options != null) {
                payload.add("options", gson.toJsonTree(options));
            }
            WireFrame frame = new WireFrame(OpCode.QUERY_AQL, payload.toString().getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, ++sequence);
            WireFrame response = sendAndWait(frame);
            return JsonParser.parseString(new String(response.payload, StandardCharsets.UTF_8))
                    .getAsJsonArray();
        } catch (IOException e) {
            throw new ThemisDBException("Query failed: " + e.getMessage());
        }
    }

    /**
     * Vector search
     */
    public JsonArray vectorSearch(String collection, double[] vector, Map<String, Object> options)
            throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("collection", collection);
            payload.add("vector", gson.toJsonTree(vector));
            Map<String, Object> opts = new HashMap<>(options != null ? options : new HashMap<>());
            opts.putIfAbsent("top_k", 10);
            opts.putIfAbsent("metric", "cosine");
            payload.add("options", gson.toJsonTree(opts));
            WireFrame frame = new WireFrame(OpCode.VECTOR_SEARCH, payload.toString().getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, ++sequence);
            WireFrame response = sendAndWait(frame);
            return JsonParser.parseString(new String(response.payload, StandardCharsets.UTF_8))
                    .getAsJsonArray();
        } catch (IOException e) {
            throw new ThemisDBException("Vector search failed: " + e.getMessage());
        }
    }

    /**
     * Geospatial query
     */
    public JsonArray geoQuery(String collection, double lat, double lon, double radiusKm,
            Map<String, Object> options) throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("collection", collection);
            payload.addProperty("lat", lat);
            payload.addProperty("lon", lon);
            payload.addProperty("radius_km", radiusKm);
            if (options != null) {
                payload.add("options", gson.toJsonTree(options));
            }
            WireFrame frame = new WireFrame(OpCode.GEO_QUERY, payload.toString().getBytes(StandardCharsets.UTF_8),
                    (short) MessageFlags.NONE, ++sequence);
            WireFrame response = sendAndWait(frame);
            return JsonParser.parseString(new String(response.payload, StandardCharsets.UTF_8))
                    .getAsJsonArray();
        } catch (IOException e) {
            throw new ThemisDBException("Geo query failed: " + e.getMessage());
        }
    }

    /**
     * Time-series query
     */
    public JsonArray timeseriesQuery(String collection, String startTime, String endTime,
            Map<String, Object> options) throws ThemisDBException {
        try {
            JsonObject payload = new JsonObject();
            payload.addProperty("collection", collection);
            payload.addProperty("start_time", startTime);
            payload.addProperty("end_time", endTime);
            if (options != null) {
                payload.add("options", gson.toJsonTree(options));
            }
            WireFrame frame = new WireFrame(OpCode.TIMESERIES_QUERY,
                    payload.toString().getBytes(StandardCharsets.UTF_8), (short) MessageFlags.NONE, ++sequence);
            WireFrame response = sendAndWait(frame);
            return JsonParser.parseString(new String(response.payload, StandardCharsets.UTF_8))
                    .getAsJsonArray();
        } catch (IOException e) {
            throw new ThemisDBException("Timeseries query failed: " + e.getMessage());
        }
    }

    // Private helper methods

    private synchronized void sendFrame(WireFrame frame) throws IOException {
        output.write(frame.toBytes());
        output.flush();
    }

    private WireFrame sendAndWait(WireFrame frame) throws IOException {
        CompletableFuture<WireFrame> future = new CompletableFuture<>();
        pendingRequests.put(frame.sequence, future);
        sendFrame(frame);

        try {
            return future.get(5, TimeUnit.SECONDS);
        } catch (TimeoutException e) {
            pendingRequests.remove(frame.sequence);
            throw new IOException("Request timeout");
        } catch (InterruptedException | ExecutionException e) {
            throw new IOException("Request failed: " + e.getMessage());
        }
    }

    private void receiveLoop() {
        byte[] buffer = new byte[1024 * 1024];
        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        while (running) {
            try {
                int bytesRead = input.read(buffer);
                if (bytesRead <= 0) break;

                baos.write(buffer, 0, bytesRead);
                byte[] data = baos.toByteArray();

                int offset = 0;
                while (offset < data.length) {
                    try {
                        WireFrame frame = WireFrame.fromBytes(data, offset);
                        offset += HEADER_SIZE + frame.payloadSize + CHECKSUM_SIZE;

                        if (frame.sequence > 0 && pendingRequests.containsKey(frame.sequence)) {
                            CompletableFuture<WireFrame> future = pendingRequests.remove(frame.sequence);
                            future.complete(frame);
                        }
                    } catch (IOException e) {
                        if (e.getMessage().contains("Incomplete")) {
                            break;
                        }
                        throw e;
                    }
                }

                baos.reset();
                if (offset > 0) {
                    baos.write(data, offset, data.length - offset);
                }
            } catch (IOException e) {
                running = false;
            }
        }
    }

    private static long calculateCRC32(byte[] data) {
        CRC32 crc = new CRC32();
        crc.update(data);
        return crc.getValue();
    }
}
