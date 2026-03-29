

const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');
const Database = require('better-sqlite3');
const cors = require('cors');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
  cors: { origin: '*', methods: ['GET', 'POST'] }
});

app.use(cors());
app.use(express.json());

// 1. Baza de date SQLite
const db = new Database(path.join(__dirname, 'comenzi.db'));

db.exec(`
  CREATE TABLE IF NOT EXISTS comenzi (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    masa INTEGER NOT NULL,
    produse TEXT NOT NULL,
    total REAL NOT NULL,
    status TEXT DEFAULT 'primita',
    robot INTEGER DEFAULT NULL,
    ora_creare TEXT NOT NULL,
    ora_actualizare TEXT
  )
`);

console.log('✅ Baza de date SQLite conectata');

// 2. Conectare Mosquitto
const mqttClient = mqtt.connect('mqtt://localhost:1883');

mqttClient.on('connect', () => {
  console.log('✅ MQTT conectat la Mosquitto local');
  mqttClient.subscribe('robot/+/status');
  mqttClient.subscribe('restaurant/raspunsuri'); 
});

mqttClient.on('message', (topic, message) => {
  const msgStr = message.toString().trim();
  console.log(`📩 Mesaj receptat: [${topic}] ${msgStr}`);

  // A. Status Robot
  if (topic.startsWith('robot/')) {
    const parts = topic.split('/');
    const robotId = parseInt(parts[1]);
    if (parts[2] === 'status') {
      io.emit('robot_status', { robotId, status: msgStr });
    }
  }

  // B. Gestionare Sosire la Masa (Logica NOUA)
  if (topic === 'restaurant/raspunsuri' && msgStr.includes("am ajuns la masa")) {
    const parti = msgStr.split(" ");
    const nrMasa = parseInt(parti[parti.length - 1]);

    console.log(`🤖 Robotul a ajuns la masa ${nrMasa}. Caut comanda în DB...`);

    const ora = new Date().toISOString();
    
    // Căutăm comanda care este 'gata' (mov) pentru acea masă
    const comandaDeLivrat = db.prepare(`
      SELECT * FROM comenzi WHERE masa = ? AND status = 'gata' LIMIT 1
    `).get(nrMasa);

    if (comandaDeLivrat) {
      db.prepare(`UPDATE comenzi SET status = 'livrat', ora_actualizare = ? WHERE id = ?`)
        .run(ora, comandaDeLivrat.id);

      console.log(`✅ [AUTOMAT] Comanda #${comandaDeLivrat.id} marcată ca LIVRATĂ.`);
      
      // TRIMITEM TOT OBIECTUL CATRE REACT
      io.emit('status_actualizat', { 
        ...comandaDeLivrat, 
        id: comandaDeLivrat.id, 
        status: 'livrat', 
        ora_actualizare: ora,
        produse: JSON.parse(comandaDeLivrat.produse) 
      });
    } else {
      console.log(`⚠️ Masa ${nrMasa} a raportat sosire, dar nu am găsit nicio comandă "Gata" (mov) în DB.`);
    }
  }
}); // <--- ACEASTA PARANTEZA LIPSEA SAU ERA PUSA GRESIT MAI SUS


mqttClient.on('error', (err) => {
  console.log('⚠️  MQTT eroare:', err.message);
});

// 3. Logica trimitere comenzi la roboti
function trimiteRobot(robotId, masa, comandaId) {
  const coduriMese = {
    1: "m",
    2: "bm",
    3: "bmmbmm"
  };

  const codTraseu = coduriMese[masa] || "m";
  const mesajPentruArduino = `robot du-te la masa ${masa} cod ${codTraseu}`;

  mqttClient.publish(`robot/${robotId}/comanda`, mesajPentruArduino, { qos: 1 });
  console.log(`🚀 [SISTEM] Comanda trimisa la robot ${robotId}: ${mesajPentruArduino}`);
}

// 4. RUTE HTTP

// Creare comanda
app.post('/comenzi', (req, res) => {
  const { masa, produse, total, ora } = req.body;

  if (!masa || !produse || !total) {
    return res.status(400).json({ eroare: 'Date incomplete' });
  }

  const stmt = db.prepare(`
    INSERT INTO comenzi (masa, produse, total, status, ora_creare)
    VALUES (?, ?, ?, 'primita', ?)
  `);

  const result = stmt.run(
    masa,
    JSON.stringify(produse),
    total,
    ora || new Date().toISOString()
  );

  const comanda = db.prepare('SELECT * FROM comenzi WHERE id = ?').get(result.lastInsertRowid);
  
  io.emit('comanda_noua', {
    ...comanda,
    produse: JSON.parse(comanda.produse),
  });

  res.json({ id: result.lastInsertRowid, status: 'primita' });
});

// Status comanda (Interogata de aplicatie la 3 secunde)
app.get('/comenzi/:id/status', (req, res) => {
  const comanda = db.prepare('SELECT status FROM comenzi WHERE id = ?').get(req.params.id);
  if (!comanda) return res.status(404).json({ eroare: 'Comanda negasita' });
  res.json({ status: comanda.status });
});

// Actualizare manuala status (din interfata administrator)
app.patch('/comenzi/:id/status', (req, res) => {
  const { status, robot } = req.body;
  const { id } = req.params;

  const statusValide = ['primita', 'preparare', 'gata', 'livrat'];
  if (!statusValide.includes(status)) {
    return res.status(400).json({ eroare: 'Status invalid' });
  }

  const ora = new Date().toISOString();
  db.prepare(`
    UPDATE comenzi SET status = ?, robot = ?, ora_actualizare = ? WHERE id = ?
  `).run(status, robot || null, ora, id);

  const comanda = db.prepare('SELECT * FROM comenzi WHERE id = ?').get(id);

  io.emit('status_actualizat', {
    id: parseInt(id),
    status,
    robot,
    ...comanda,
    produse: JSON.parse(comanda.produse),
  });

  // Daca adminul marcheaza "gata", robotul pleaca
  if (status === 'gata' && robot) {
    trimiteRobot(robot, comanda.masa, id);
  }

  res.json({ success: true, status });
});

// Sterge comanda
app.delete('/comenzi/:id', (req, res) => {
  db.prepare('DELETE FROM comenzi WHERE id = ?').run(req.params.id);
  io.emit('comanda_stearsa', { id: parseInt(req.params.id) });
  res.json({ success: true });
});

// ... restul rutelor tale (meniu, roboti) ramân neschimbate ...
app.get('/meniu', (req, res) => {
  try {
    const meniu = require('../recomandari_AI/meniu.json');
    res.json(meniu);
  } catch (e) {
    res.status(404).json({ eroare: 'Meniu negasit' });
  }
});

app.get('/roboti', (req, res) => {
  res.json([
    { id: 1, nume: 'Robot 1', status: 'liber' },
    { id: 2, nume: 'Robot 2', status: 'liber' },
    { id: 3, nume: 'Robot 3', status: 'liber' },
  ]);
});

// 5. Socket.io
/*io.on('connection', (socket) => {
  console.log(`📱 Client conectat: ${socket.id}`);
  const comenziActive = db.prepare("SELECT * FROM comenzi WHERE status != 'livrat' ORDER BY id DESC").all();
  socket.emit('comenzi_initiale', comenziActive.map(c => ({ ...c, produse: JSON.parse(c.produse) })));
  socket.on('disconnect', () => console.log(`📱 Client deconectat: ${socket.id}`));
});*/

io.on('connection', (socket) => {
  const azi = new Date().toISOString().split('T')[0];
  const comenziAzi = db.prepare(
    `SELECT * FROM comenzi WHERE ora_creare LIKE ? ORDER BY id DESC`
  ).all(`${azi}%`);
  socket.emit('comenzi_initiale', comenziAzi.map(c => ({ 
    ...c, produse: JSON.parse(c.produse) 
  })));
  socket.on('disconnect', () => console.log(`📱 Client deconectat: ${socket.id}`));
});
const PORT = 3000;
server.listen(PORT, '0.0.0.0', () => {
  console.log(`\n🚀 SERVER GLOBAL ACTIV`);
  console.log(`📱 Acces iPhone/Robot: http://172.20.10.2:${PORT}`);
  console.log(`------------------------------------------`);
});