const { useState } = React;

function DrinkButton({drinkName, price, onOrder}) {
  const [count, setCount] = useState(0);

  const handleClick = () => {
    // Increment the counter when the button is clicked
    setCount(prevCount => prevCount + 1);
    onOrder(price);
  };

  return (
          <div className="drink-item">
            <button id={drinkName.toLowerCase()} onClick={handleClick}>
              <span className="drink-name">{drinkName}</span>
              <span className="price">{price} yen</span>
            </button>
            <div className="counter" id={`${drinkName.toLowerCase()}-count`}>{count}</div>
          </div>
  );
}

function App() {
  const [totalItems, setTotalItems] = useState(0);
  const [totalPrice, setTotalPrice] = useState(0);

  const handleOrder = (price) => {
    setTotalItems(prevItems => prevItems + 1);
    setTotalPrice(prevPrice => prevPrice + price);
  };

  return (
    <div className="app-container">
      <div className="drink-buttons">
        <DrinkButton drinkName="Coffee" price={480} onOrder={handleOrder} />
        <DrinkButton drinkName="Tea" price={280} onOrder={handleOrder} />
        <DrinkButton drinkName="Milk" price={180} onOrder={handleOrder} />
        <DrinkButton drinkName="Coke" price={190} onOrder={handleOrder} />
        <DrinkButton drinkName="Beer" price={580} onOrder={handleOrder} />
      </div>
      <div className="order-summary">
        <h2>Your bill</h2>
          <hr className="summary-separator" />
          <div className="summary-item">Item ordered: <span id="count">{totalItems}</span></div>
          <div className="summary-item">Total Price: <span id="price">{totalPrice}</span> yen</div>
      </div>
    </div>
  );
}

ReactDOM.render(<App />, document.getElementById('root'));
