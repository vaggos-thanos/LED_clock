require('dotenv').config();
const API = require('./classes/Api.js');


const api = new API(process.env.API_URL);
api.start()
    .then(() => {
        api.functions.log('API started successfully');
    })
    .catch((error) => {
        api.functions.log('Error starting API:', error);
    });
