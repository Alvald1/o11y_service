import express from "express";
import { faker } from "@faker-js/faker";

const app = express();
const port = process.env.PORT || 3000;

app.get("/user", (req, res) => {
    const user = {
        id: faker.string.uuid(),
        name: faker.person.fullName(),
        email: faker.internet.email(),
        avatar: faker.image.avatar(),
        birthday: faker.date.birthdate(),
        address: faker.location.streetAddress(),
    };
    res.json(user);
});

app.get("/", (req, res) => {
    res.send("Faker API server is running!");
});

app.listen(port, () => {
    console.log(`Faker server listening on port ${port}`);
});
