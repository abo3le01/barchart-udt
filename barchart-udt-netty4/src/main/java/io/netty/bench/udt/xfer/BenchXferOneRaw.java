/*
 * Copyright 2012 The Netty Project
 *
 * The Netty Project licenses this file to you under the Apache License,
 * version 2.0 (the "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */
package io.netty.bench.udt.xfer;

import static io.netty.bench.udt.util.UnitHelp.*;
import io.netty.example.udt.util.ConsoleReporterUDT;

import java.nio.ByteBuffer;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.barchart.udt.SocketUDT;
import com.barchart.udt.StatusUDT;
import com.barchart.udt.TypeUDT;
import com.yammer.metrics.Metrics;
import com.yammer.metrics.core.Counter;
import com.yammer.metrics.core.Meter;
import com.yammer.metrics.core.Timer;
import com.yammer.metrics.core.TimerContext;

/**
 * perform one way raw send/recv in 2 threads
 */
public final class BenchXferOneRaw {

    private BenchXferOneRaw() {
    }

    static final Logger log = LoggerFactory.getLogger(BenchXferOneRaw.class);

    /** benchmark duration */
    static final int time = 60 * 1000;

    /** transfer chunk size */
    static final int size = 64 * 1024;

    static final Counter benchTime = Metrics.newCounter(BenchXferOneRaw.class,
            "bench time");

    static final Counter benchSize = Metrics.newCounter(BenchXferOneRaw.class,
            "bench size");

    static {
        benchTime.inc(time);
        benchSize.inc(size);
    }

    static final Meter sendRate = Metrics.newMeter(BenchXferOneRaw.class,
            "send rate", "bytes", TimeUnit.SECONDS);

    static final Timer sendTime = Metrics.newTimer(BenchXferOneRaw.class,
            "send time", TimeUnit.MILLISECONDS, TimeUnit.SECONDS);

    public static void main(final String[] args) throws Exception {

        log.info("init");

        final SocketUDT accept = new SocketUDT(TypeUDT.DATAGRAM);
        accept.configureBlocking(true);
        accept.bind(localSocketAddress());
        accept.listen(1);
        socketAwait(accept, StatusUDT.LISTENING);
        log.info("accept : {}", accept);

        final SocketUDT client = new SocketUDT(TypeUDT.DATAGRAM);
        client.configureBlocking(true);
        client.bind(localSocketAddress());
        socketAwait(client, StatusUDT.OPENED);
        client.connect(accept.getLocalSocketAddress());
        socketAwait(client, StatusUDT.CONNECTED);
        log.info("client : {}", client);

        final SocketUDT server = accept.accept();
        server.configureBlocking(true);
        socketAwait(server, StatusUDT.CONNECTED);
        log.info("server : {}", server);

        final AtomicBoolean isOn = new AtomicBoolean(true);

        final Runnable clientTask = new Runnable() {

            @Override
            public void run() {
                try {
                    while (isOn.get()) {
                        runCore();
                    }
                } catch (final Exception e) {
                    log.error("", e);
                }
            }

            final ByteBuffer buffer = ByteBuffer.allocateDirect(size);

            long sequence;

            void runCore() throws Exception {

                buffer.rewind();
                buffer.putLong(0, sequence++);

                final TimerContext timer = sendTime.time();

                final int count = client.send(buffer);

                timer.stop();

                if (count != size) {
                    throw new Exception("count");
                }

                sendRate.mark(count);
            }
        };

        final Runnable serverTask = new Runnable() {

            @Override
            public void run() {
                try {
                    while (isOn.get()) {
                        runCore();
                    }
                } catch (final Exception e) {
                    log.error("", e);
                }
            }

            final ByteBuffer buffer = ByteBuffer.allocateDirect(size);

            long sequence;

            void runCore() throws Exception {

                buffer.rewind();

                final int count = server.receive(buffer);

                if (count != size) {
                    throw new Exception("count");
                }

                if (this.sequence++ != buffer.getLong(0)) {
                    throw new Exception("sequence");
                }
            }
        };

        final ExecutorService executor = Executors.newFixedThreadPool(2);

        executor.submit(clientTask);

        executor.submit(serverTask);

        ConsoleReporterUDT.enable(3, TimeUnit.SECONDS);

        Thread.sleep(time);

        isOn.set(false);

        Thread.sleep(1 * 1000);

        executor.shutdownNow();

        Metrics.defaultRegistry().shutdown();

        accept.close();
        client.close();
        server.close();

        log.info("done");
    }

}
