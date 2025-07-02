const colors = require('ansi-colors');
const fs = require('fs');
const path = require('path');

class db_handler {

    constructor(client, mysql, toggle) {
        this.mysql = mysql;
        this.toggle = toggle ? toggle : true;
        this.client = client;
    }

    async setStatus(status, message, error) {
        status = status ? true : false;
        message = message ? message : "No message provided";
        error = error ? error : "No error provided";
        this.statusData = {
            status,
            message,
            error
        }
    }

    async checkSQLFile() {
        await this.setStatus(false, "stating sql file checks and backup")
        log('[+] Checking SQL file...', this.toggle, 'green');
        const sqlPath = path.join(__dirname, '../../api.sql');
        const sqlContent = fs.readFileSync(sqlPath, 'utf8');
        const tables = sqlContent.split(';').filter(statement => statement.includes('create table'));
    
        const apiJSON = path.join(__dirname, '../../apiDB.json');
        if (fs.existsSync(apiJSON)) {
            log('[+] JSON file exists!', this.toggle, 'green');
            const jsonContent = fs.readFileSync(apiJSON, 'utf8');
            const json = JSON.parse(jsonContent);
            
            let newJson = [];
            for (const table of tables) {
                const tableName = table.split('(')[0].split(' ')[2].trim();
                let sqlInsert = table.trim();
                sqlInsert = sqlInsert.replace(/(\r\n|\n|\r)/gm, ' ');
                newJson.push({
                    tableName,
                    sqlInsert
                })
            }

            if (JSON.stringify(json) === JSON.stringify(newJson)) {
                log('[+] JSON file is up to date!', this.toggle, 'green');
                return;
            }

            log('[+] JSON file is outdated. Creating a new one...', this.toggle, 'yellow');
            
            fs.writeFileSync(apiJSON, JSON.stringify(newJson));
            log(`[+] JSON file created: ${apiJSON}`, this.toggle, 'green');
            await this.setStatus(false, "Finished sql file checks and backup all good!")
        }
    }

    async login(host, username, password, database) {
        await this.setStatus(false, "Initialazing", "none")
        await this.checkSQLFile();
        const apiJSON = import(path.join(__dirname, '../../apiDB.json'));

        let while_v = true

        let db = this.mysql.createPool({
            host: host,
            user: username,
            password: password,
            database: database
        })
        await this.setStatus(false, "Sql has been conncted")
        db.query('SELECT * FROM INFORMATION_SCHEMA.TABLES', async (err, rows) => {
            if (err) {
                log('Failed to login! ❌', this.toggle, 'red')
                await this.setStatus(false, "Sql failed to connect... Retrying!", "Failed connection")
                // throw err;
                db = this.mysql.createPool({
                    host: host,
                    user: username,
                    password: password,
                })
                await this.setStatus(false, "Sql connected! Doing table integrity testing!")
                let creatingDB = true
                db.query('CREATE DATABASE IF NOT EXISTS ' + database, async (err, rows) => {
                    if (err) {
                        throw err;
                    }
                    log('[+] Database created!', this.toggle, 'green');
                    
                    // db.close();
                    db = await this.mysql.createPool({
                        host: host,
                        user: username,
                        password: password,
                        database: database
                    })
                    while (db.config.connectionConfig.database != database) {
                        await this.client.functions.sleep(100)
                    }
                    log('[+] Connected to database', this.toggle , 'green');
                    log('[+] Inserting tables...', this.toggle, 'green');
                    
                    let db_creatorflag = false
                    for (const sql of apiJSON) {
                        await this.create_table(sql.sqlInsert, db)
                        log('[+] Inserted Table: ' + sql.tableName, this.toggle, 'yellow')
                        db_creatorflag = true
                    }
                    while (!db_creatorflag) {
                        await this.client.functions.sleep(100);
                    }
                    log('[+] Tables inserted!', this.toggle, 'green');

                    creatingDB = false
                })

                while (creatingDB){
                    await this.client.functions.sleep(10)
                }
            }


            for (const table of apiJSON) {
                if (!await this.check_table(table.tableName, db)) {
                    log('Missing table: ' + table.tableName, this.toggle, 'red')
                    console.log(table)
                    await this.create_table(table.sqlInsert, db)
                }
            }

            this.db = db;
            this.database_name = database;

            log('[+] Connected to database', this.toggle , 'green');
            await this.setStatus(true, "SQL has been initialized correctly and is ready!")
            while_v = false
        })

        while(while_v) {
            await sleep(10)
        }
        
        return db;
    }

    async create_table(insert, db) {
        if (!db) {
            db = this.db;
        }

        let while_v = true
        let code = []
        if (typeof insert === 'object') {
            for (const sqlInsert of insert) {
                db.query(sqlInsert.sqlInsert, (err, rows) => {
                    if (err) throw err;
                    code.push({
                        text: 'Done!', 
                        value: true,
                        data: undefined
                    })
                })
            }
            while_v = false
        } else {
            db.query(insert, (err, rows) => {
                if (err) throw err;
                code.push({
                    text: 'Done!', 
                    value: true,
                    data: undefined
                })
                while_v = false
            })
        }

        while(while_v) {
            await sleep(10)
        }

        return code[0];
    }

    async init(db_names, local_dbs) {
        let counter = 0
        while(counter < db_names.length) {
            const db_data = await this.get_all_rows(db_names[counter]);
            let counter1 = 0
            while(counter1 < db_data.length) {
                let key;
                const data = db_data[counter1];
                if(db_names[counter] == 'GuildSettingsFiveMServers' ) {
                    key = data.server_ip
                } else {   
                    key = data.guild_id
                }
                local_dbs[counter].set(key, data);
                counter1++
            }
            counter++;              
        }
        log('[+] Done!', true, 'green');
        return {done: true}
    }

    async check_table(table_name, db) {
        return new Promise((resolve, reject) => {
            if (!db) {
                db = this.db;
            }
            db.query(`SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = '${table_name}'`, (err, rows) => {
                if (err) {
                    reject(err);
                }
                if(rows.length == 0) {
                    resolve(false);
                } else {
                    resolve(true);
                }
            })
        })
    }

    // check_table_content(table_name, key_name, key_value) {
        
    // }

    async get_all_rows(table_name) {
        let while_v = true
        let Rows = []

        this.db.query(`SELECT * FROM ${table_name}`, (err, rows) => {
            if (err) {
                throw err;
            }
            while_v = false
            Rows.push(rows);
        })

        while(while_v) {
            await sleep(10)
        }

        return Rows[0];
    }

    async get_row(table_name, key_name, key_value) {
        let while_v = true
        let Rows = []

        this.db.query(`SELECT * FROM ${table_name} WHERE ${key_name} = ${key_value}`, (err, rows) => {
            if (err) throw err;
            while_v = false
            Rows.push(rows);
        })

        while(while_v) {
            await sleep(10)
        }
        return Rows[0][0];
    }

    async create_row(table_name, values_keys, values, ignore) {
        let while_v = true
        let code = []
        values_keys = values_keys != null ? `(${values_keys})` : ' '
        let ignore_v = ignore ? 'IGNORE' : ' '
        this.db.query(`INSERT ${ignore_v} INTO ${table_name} ${values_keys} VALUES (${values})`, (err, rows) => {
            if (err) throw err;
            code.push({
                text: 'Done!', 
                value: true,
                data: [values]
            })
            while_v = false
        })

        while(while_v) {
            await sleep(10)
        }
        return code[0];
    }

    async update_row(table_name, set_value, value, key_name, key_value) {
        let while_v = true
        let code = []
        let set = ''
        if(set_value.split(',').length > 1) {
            const set_values = set_value.split(',')
            const values = value.split(',')
            let comma = ','
            for (const word of set_values) {
                if(set_values.indexOf(word) == set_values.length - 1) comma = ''
                set += `${word} = "${values[set_values.indexOf(word)]}"${comma}`
            }
        } else {
            set = `${set_value} = '${value}'`
        }

        this.db.query(`UPDATE IGNORE ${table_name} SET ${set} WHERE ${key_name} = ${key_value}`, (err, rows) => {
            if (err) throw err;
            this.db.query(`SELECT * FROM ${table_name} WHERE ${key_name} = ${key_value}`, (err, rows) => {
                code.push({
                    text: 'Done!', 
                    value: true,
                    data: rows[0]
                })
                while_v = false
            })
        })
        
        while(while_v) {
            await sleep(10)
        }

        return code[0];
    }

    async delete_row(table_name, key_name, key_value) {
        let while_v = true
        let code = []

        this.db.query(`DELETE IGNORE FROM ${table_name} WHERE ${key_name} = ${key_value}`, (err, rows) => {
            if (err) throw err;
            code.push({
                text: 'Done!', 
                value: true,
                data: undefined
            })
            while_v = false
        })

        while(while_v) {
            await sleep(10)
        }

        return code[0];
    }

    async query(query) {
        let while_v = true
        let Rows = []

        this.db.query(query, (err, rows) => {
            if (err) {
                throw err;
            }
            while_v = false
            Rows.push(rows);
        })

        while(while_v) {
            await sleep(10)
        }

        return Rows[0];
    }
}

function log(msg, toggle, color_) {
    let color = color_ ? color_ : 'white';

    if(toggle) {
        console.log(colors[color](msg));
    }
}

async function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

module.exports = db_handler;