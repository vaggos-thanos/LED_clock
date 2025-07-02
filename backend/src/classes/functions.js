const colors = require('ansi-colors');
const os = require('os');

class functions {
    formatDate(date) {
        var day = date.getDate();
        var month = date.getMonth() + 1;
        var year = date.getFullYear();
        var hours = date.getHours();
        var mins  = date.getMinutes();
        var secs  = date.getSeconds();
        
        day = (day < 10 ? "0" : "") + day;
        month = (month < 10 ? "0" : "") + month;
        year = (year < 10 ? "0" : "") + year;
        hours = (hours < 10 ? "0" : "") + hours;
        mins = (mins < 10 ? "0" : "") + mins;
        secs = (secs < 10 ? "0" : "") + secs;

        return `${hours}:${mins}:${secs} ${day}/${month}/${year}`;
    }

    async log(str, error) {
        if(error) {
            console.log(colors.red(`[${this.formatDate(new Date())}] ${str}`));
            console.log(error);
        } else {
            console.log(`${colors.cyan(`[${this.formatDate(new Date())}]:`)} ${str}`);
        }
        
    }

    async sleep(ms) {
        return new Promise((resolve) => {
            setTimeout(resolve, ms);
        });
    }

    async isOwner(member) {
        try {
            const users = ['667357315950706704']

            const state = users.reduce((result, user) => {
                return result || user == member
            }, false);
            return state;

        } catch (error) {
           this.log(error, error)
            return false
        }
    }

    async isAuthor(id) {
        try {
            const authors = ['588416409407848457'] /*Vaggos[1] */
            if (authors[0] == id) {
                return true
            }

            return false
        } catch (error) {
           this.log(error, error)
            return false
        }
    }

    async parseURL(url) {
        try {
            let parsedUrl = url.split('//')[1]
            let endpoint = '/' + parsedUrl.split('/')[1];
            parsedUrl = parsedUrl.split(':')[0];
            if (parsedUrl.includes("0.0.0.0")) {
                const interfaces = os.networkInterfaces();
                for (const name of Object.keys(interfaces)) {
                    if (name === "Ethernet" || name === "Wi-Fi") {
                        const addresses = interfaces[name];
                        for (const address of addresses) {
                            if (address.family === 'IPv4' && !address.internal) {
                                parsedUrl = address.address;
                            }
                        }
                    }
                }
            }
            parsedUrl = `http://${parsedUrl}${endpoint}`;
            return parsedUrl;
        } catch (error) {
            this.log(`Error parsing URL: ${url}`, error);
            return null;
        }
    }
}

module.exports = functions;