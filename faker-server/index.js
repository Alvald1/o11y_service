import express from "express";
import { faker } from "@faker-js/faker";
import winston from "winston";


const logger = winston.createLogger({
    level: "info",
    format: winston.format.combine(
        winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
        winston.format.printf(({ timestamp, level, message }) => {
            // Crow/Spdlog style: (YYYY-MM-DD HH:MM:SS) [LEVEL   ] message
            const padLevel = level.toUpperCase().padEnd(8, ' ');
            return `(${timestamp}) [${padLevel}] ${message}`;
        })
    ),
    transports: [
        new winston.transports.Console()
    ]
});

const app = express();
const port = process.env.PORT || 3000;


app.get("/user", (req, res) => {
    try {
        const user = {
            id: faker.string.uuid(),
            name: faker.person.fullName(),
            email: faker.internet.email(),
            avatar: faker.image.avatar(),
            birthday: faker.date.birthdate(),
            address: faker.location.streetAddress(),
        };
        logger.info(`Generated user: ${JSON.stringify(user)}`);
        res.json(user);
    } catch (err) {
        logger.error(`Error generating user: ${err}`);
        res.status(500).json({ error: "Internal server error" });
    }
});


app.get("/", (req, res) => {
    logger.info("Root endpoint called");
    res.send("Faker API server is running!");
});


app.listen(port, () => {
    logger.info(`Faker server listening on port ${port}`);
});
