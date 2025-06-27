async function loadGigs()
{
    try
    {
        const res = await fetch('/gigs');
        const data = await res.json();
        const phase = data.phase;
        const list = document.getElementById('gigList');
        list.innerHTML = '';
        data.gigs.forEach(g => {
            if (g.phaseId !== phase)
            {
                return;
            }
            const row = document.createElement('div');
            row.textContent = g.title;
            if (g.completed)
            {
                row.style.opacity = '0.5';
                if (g.phaseId !== phase)
                {
                    row.onclick = () => window.nk && window.nk.call('OpenAssistMenu', g.phaseId);
                }
            }
            else
            {
                row.onclick = () => window.nk && window.nk.call('TrackGig', g.questHash);
            }
            list.appendChild(row);
        });
    }
    catch (e)
    {
        console.error(e);
    }
}
window.addEventListener('DOMContentLoaded', loadGigs);
