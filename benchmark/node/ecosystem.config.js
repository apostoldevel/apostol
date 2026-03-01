module.exports = {
  apps: [{
    name: 'bench-node',
    script: './app.js',
    instances: parseInt(process.env.WORKERS || '2', 10),
    exec_mode: 'cluster',
    autorestart: true,
    watch: false,
    max_memory_restart: '512M',
  }],
};
