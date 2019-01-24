import asyncio
import time
import serial_asyncio
import sys
import os


from display import render
from processor import Processor
from command_processor import COMMAND_LIST
from checker import create_messenger_open_close_checks


OUTPUT_DIR = 'output'
OUTPUT_DASHBOARD_PATH = os.path.join(OUTPUT_DIR, 'dashboard')
OUTPUT_NODES_PATH = os.path.join(OUTPUT_DIR, 'nodes')
OUTPUT_PROTOCOL_TRANSCRIPT_PATH = os.path.join(
    OUTPUT_DIR, 'protocol_transcript')
OUTPUT_CONSOLE_TRANSCRIPT_PATH = os.path.join(
    OUTPUT_DIR, 'console_transcript')

CHECKS = \
    create_messenger_open_close_checks(
        'Main', 'Main door', os.environ['FB_OWNER_ID']) + \
    create_messenger_open_close_checks(
        'Bathroom', 'Bathroom door', os.environ['FB_OWNER_ID'])

SESSION_GAP_THRESHOLD = 10.0

LEFT_MARGIN = 38

REPLAY = True


class ConsoleSerial(asyncio.Protocol):
    def __init__(self, tx_queue, rx_queue, processor):
        self.tx_queue = tx_queue
        self.rx_queue = rx_queue
        self.reply_pending = False
        self.processor = processor

    def connection_made(self, transport):
        self.transport = transport
        self.buffer = b''

        asyncio.get_event_loop().create_task(self._process_outgoing())

    def data_received(self, data):
        self.buffer += data

        while True:
            found = self.buffer.find(b'\r\n')
            if found < 0:
                break

            message = self.buffer[:found].decode()

            timestamp = time.time()
            with open(OUTPUT_PROTOCOL_TRANSCRIPT_PATH, 'a') as f:
                f.write('{} {}\n'.format(str(timestamp), message))

            self.rx_queue.put_nowait((timestamp, False, message))

            self.buffer = self.buffer[found + 2:]

    async def _process_outgoing(self):
        while True:
            request = await self.tx_queue.get()
            self.transport.write((request + '\n').encode())


async def transcribe_and_process_console_message(processor, message):
    timestamp = time.time()
    with open(OUTPUT_CONSOLE_TRANSCRIPT_PATH, 'a') as f:
        f.write('{} {}\n'.format(str(timestamp), message))

    await processor.process_console_message(timestamp, message)


async def interact(processor):
    await transcribe_and_process_console_message(processor, 'session_reset')

    while True:
        sys.stdout.write('\n')
        sys.stdout.write('> ')
        sys.stdout.flush()

        message = await (asyncio.get_event_loop()
                                .run_in_executor(None, sys.stdin.readline))
        message = message[:-1]

        await transcribe_and_process_console_message(processor, message)


async def display(processor):
    while True:
        with open(OUTPUT_DASHBOARD_PATH, 'w') as f:
            f.write(render(processor.nodes))

        await asyncio.sleep(1.0)


async def replay(processor):
    if not os.path.exists(OUTPUT_PROTOCOL_TRANSCRIPT_PATH):
        return

    print("Replaying transcript...")

    time_begin = time.time()
    entries = []

    with open(OUTPUT_PROTOCOL_TRANSCRIPT_PATH, 'r') as f:
        for line in f:
            timestamp, *rest = line.split()
            timestamp = float(timestamp)
            message = ' '.join(rest)
            entries.append((timestamp, 'protocol', message))

    with open(OUTPUT_CONSOLE_TRANSCRIPT_PATH, 'r') as f:
        for line in f:
            timestamp, *rest = line.split()
            timestamp = float(timestamp)
            message = ' '.join(rest)
            entries.append((timestamp, 'console', message))

    last_timestamp = None

    for timestamp, source, message in sorted(entries):
        if (last_timestamp is not None) and \
           (timestamp >= last_timestamp + SESSION_GAP_THRESHOLD):
            await processor.process_console_message(timestamp, 'session_reset')
        last_timestamp = timestamp

        if source == 'protocol':
            if message.startswith("sta "):
                await processor.protocol_rx_queue.put(
                    (timestamp, True, message))
                await processor.protocol_rx_queue.join()
        elif source == 'console':
            op = message.split()[0]
            if op in COMMAND_LIST:
                await processor.process_console_message(timestamp, message)

    print('Replayed {} entries in {:.1f} seconds.'.format(
        len(entries),
        time.time() - time_begin))


async def main():
    tx_queue = asyncio.Queue()
    rx_queue = asyncio.Queue()

    processor = Processor(tx_queue, rx_queue, CHECKS)
    processor.start()

    if REPLAY:
        await replay(processor)

    os.makedirs(OUTPUT_DIR, 0o777, True)

    await asyncio.gather(
        serial_asyncio.create_serial_connection(
            loop,
            lambda: ConsoleSerial(tx_queue, rx_queue, processor),
            '/dev/cu.usbmodem401111',
            baudrate=115200),
        display(processor),
        interact(processor))


loop = asyncio.get_event_loop()
loop.run_until_complete(main())
loop.close()
