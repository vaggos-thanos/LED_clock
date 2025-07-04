const { EntitySchema } = require("typeorm");

module.exports = new EntitySchema({
    name: "RateLimit",
    tableName: "rate_limits",
    columns: {
        key: {
            primary: true,
            type: "varchar",
        },
        totalHits: {
            type: "int",
        },
        resetTime: {
            type: "timestamp",
        },
    },
});