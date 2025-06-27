const apartments = [ {id : 1, price : 15000}, {id : 2, price : 20000} ];
function send(id)
{
    if (window.nk)
    {
        window.nk.call('CoopPurchaseApt', id);
    }
}
function render()
{
    const root = document.getElementById('aptList');
    apartments.forEach(a => {
        const row = document.createElement('div');
        const buy = document.createElement('button');
        buy.textContent = 'Buy';
        buy.onclick = () => send(a.id);
        row.textContent = `Apartment ${a.id} - $${a.price}`;
        row.appendChild(buy);
        root.appendChild(row);
    });
}
window.addEventListener('DOMContentLoaded', render);
