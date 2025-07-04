const express = require('express');
const { Server } = require('socket.io');
const path = require('path');
const fs = require('fs');
const cors = require('cors');
const { createServer } = require('node:http');
const mysql = require('mysql2');

const db_handler = require('./dbManager.js');
const functions = require('./functions.js');
const { DataSource } = require('typeorm');
const { ExpressRateLimitTypeOrmStore } = require('typeorm-rate-limit-store');
const rateLimit = require('express-rate-limit');
const RateLimitEntity = require('../entities/RateLimit.js'); // Adjust the path to your RateLimit entity


class API {
    constructor(url) {
        this.url = url
        this.app = express();
        this.server = createServer(this.app);
        this.io = new Server(this.server, {
            cors: {
                origin: '*',
                methods: ['GET', 'POST'],
                allowedHeaders: ['Content-Type'],
                credentials: true
            }
        });

        this.app.use(express.json());
        this.app.use(express.urlencoded({ extended: true }));
        this.app.use(cors());

        this.sockets = new Map();

        this.dbManager = new db_handler(this, mysql);
        this.functions = new functions(this);
    }

    async setupRoutes() {
        this.app.get('/api', (req, res) => {
            res.send('<h1>Hello papia</h1>');
        });
    }

    async setupSocket() {
        this.loadSocketEvents("../sockets");
        this.io.on('connection', (socket) => {
            this.functions.log('A user connected:', socket.id);

            socket.on('disconnect', () => {
                this.functions.log('User disconnected:', socket.id);
            });

            // Add more socket event handlers as needed
            socket.on('message', (data) => {
                this.functions.log('Message received:', data);
                socket.emit('message', { message: 'Message received' });

                const type = data.type;
                if (this.sockets.has(type)) {
                    const event = this.sockets.get(type);
                    event.handle(socket, data);
                } else {
                    this.functions.log(`No socket event handler found for type: ${type}`);
                }
            });

        });
    
    }

    async loadSocketEvents(dir) {
        const sockets = fs.readdirSync(path.join(__dirname, dir));
        for (const file of sockets) {
            if (file.endsWith('.js')) {
                const socket = new (require(path.join(__dirname, dir, file)))(this);
                if (socket.name !== file.split(".ts")[0] && socket.name !== file.split(".js")[0]) return this.functions.log(`Socket name mismatch: ${file} vs ${socket.name}`);
                await this.sockets.set(socket.name, socket);
                this.functions.log(`Loaded socket: ${socket.name}`);
            } else if (fs.lstatSync(path.join(__dirname, dir, file)).isDirectory()) {
                this.functions.log(`Skipping directory: ${file}`);
            } else {
                this.functions.log(`Skipping file: ${file}`);
            }
        }
    }

    async loadRoutes() {}

    async start() {
        return new Promise(async (resolve, reject) => {
            try {
                this.url = await this.functions.parseURL(this.url);

                await this.dbManager.login(
                    process.env.DB_HOST,
                    process.env.DB_USER,
                    process.env.DB_PASS,
                    process.env.DB_DATA
                );

                const AppDataSource = new DataSource({
                    type: "mysql", // works with MariaDB too
                    host: process.env.DB_HOST,
                    port: 3306,
                    username: process.env.DB_USER,
                    password: process.env.DB_PASS,
                    database: process.env.DB_DATA,
                    synchronize: true,
                    entities: [RateLimitEntity], // Adjust the path to your entities
                });

                await AppDataSource.initialize();

                this.limiter = rateLimit({
                    windowMs: 2 * 60 * 1000, // 15 minutes
                    max: 200, // Limit each IP to 5 requests per windowMs
                    standardHeaders: true,
                    legacyHeaders: false,
                    store: new ExpressRateLimitTypeOrmStore(AppDataSource.getRepository(RateLimitEntity), "users_"),
                });

                this.app.use(this.limiter);

                await this.setupRoutes();
                await this.setupSocket();

                await this.server.listen(process.env.PORT || 3000, () => {
                    this.functions.log(`API is running at ${this.url}`);
                    resolve();
                });

                await this.server.on('error', (error) => {
                    this.functions.error('Error starting server:', error);
                    reject(error);
                });
            } catch (error) {
                reject(error);
            }
        });
    }

}

module.exports = API;