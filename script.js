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

    const MathRandom = Math.random;
    function createMeteor() {
        return {
            x: MathRandom() * (width + height) - height,
            y: MathRandom() * height * 2 - height,
            length: MathRandom() * 60 + 15,
            speed: MathRandom() * 10 + 5,
            opacity: MathRandom() * 0.5 + 0.1,
            weight: MathRandom() * 1.5 + 0.5
        };
    }

    const meteors = Array.from({length: 80}, createMeteor);
    let lastFrame = 0;

    function render(time) {
        requestAnimationFrame(render);
        if (time - lastFrame < 1000 / 30) return;
        lastFrame = time;
        
        ctx.clearRect(0, 0, width, height);
        
        for (let i = 0; i < meteors.length; i++) {
            let m = meteors[i];
            
            m.x += m.speed;
            m.y += m.speed;
            
            if (m.x > width + m.length || m.y > height + m.length) {
                m.x = MathRandom() * (width + height) - height;
                m.y = -m.length - MathRandom() * 100;
                m.speed = MathRandom() * 10 + 5;
            }
            
            ctx.beginPath();
            ctx.strokeStyle = `rgba(255, 255, 255, ${m.opacity})`;
            ctx.lineWidth = m.weight;
            ctx.moveTo(m.x, m.y);
            ctx.lineTo(m.x - m.length, m.y - m.length);
            ctx.stroke();
        }
    }
    requestAnimationFrame(render);
});