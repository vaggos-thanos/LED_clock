class DebugSocket {
    constructor(api) {
        this.api = api;
        this.name = 'debug';
    }

    handle(socket, data) {
        console.log(`DebugSocket received data: ${JSON.stringify(data)}`);
        socket.emit('debug', { message: 'DebugSocket response' });
    }
}

module.exports = DebugSocket;
