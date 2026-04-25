document.addEventListener("DOMContentLoaded", () => {
    const canvas = document.getElementById('starfield');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    let width, height;
    
    function resize() {
        width = canvas.width = window.innerWidth;
        height = canvas.height = window.innerHeight;
    }
    window.addEventListener('resize', resize);
    resize();

    const stars = Array.from({length: 250}, () => ({ x: Math.random() * 2000 - 1000, y: Math.random() * 2000 - 1000, z: Math.random() * 2000 }));
    let lastFrame = 0;

    function render(time) {
        requestAnimationFrame(render);
        if (time - lastFrame < 1000 / 30) return;
        lastFrame = time;
        ctx.clearRect(0, 0, width, height);
        ctx.fillStyle = 'rgba(255, 255, 255, 0.7)';
        const cx = width / 2; const cy = height / 2;
        for (let i = 0; i < stars.length; i++) {
            let star = stars[i];
            star.z -= 10;
            if (star.z <= 0) { star.z = 2000; star.x = Math.random() * 2000 - 1000; star.y = Math.random() * 2000 - 1000; }
            let px = cx + (star.x / star.z) * 1000;
            let py = cy + (star.y / star.z) * 1000;
            if (px >= 0 && px <= width && py >= 0 && py <= height) {
                ctx.beginPath(); ctx.arc(px, py, (1 - star.z / 2000) * 2, 0, 6.28); ctx.fill();
            }
        }
    }
    requestAnimationFrame(render);
});