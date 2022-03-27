let web3;

let tokenFrom;
let tokenTo;
let lpToken;

let tokenFromDecimals;
let tokenToDecimals;
let lpTokenDecimals;

let userWallet;

const decimalsToEther = (function (decimals) {
    const unitMap = Web3.utils.unitMap;
    const keys = Object.keys(unitMap);
    const decimalsToFunnyEtherNames = {};
    for (unit of keys) {
        decimalsToFunnyEtherNames[unitMap[unit].length - 1] = unit;
    }

    return decimalsToFunnyEtherNames;
})();

function loadBalances(account) {
    const ethereum = window.ethereum;
    const balance = document.getElementById("balance");

    ethereum.request({method: 'eth_getBalance', params: [account, 'latest']})
        .then(balance => {
            const bal = document.getElementById("balance");
            const utils = Web3.utils;

            bal.innerHTML = utils.fromWei(utils.hexToNumberString(balance), "ether") + " AEV";
        })
        .catch(error => {
            console.error(error.message);
        });
}

function requestAEV() {

}

function walletChange(accounts) {
    const request_button = document.getElementById("request_button");
    request_button.disabled = false;

    if (accounts.length === 0) {
        // Metamask not connected
        request_button.innerHTML = "Connect Wallet";
        request_button.addEventListener("click", connectWallet);
        return;
    }

    // Metamask connected. Enable faucet and load balances
    const balance = document.getElementById("balance");

    userWallet = accounts[0];
    request_button.innerHTML = "Request AEV";
    request_button.addEventListener("click", requestAEV);

    balance.innerHTML = "Retrieving AEV balance...";
    loadBalances(userWallet);


    /*const abridged = `${userWallet.slice(0, 6)}...${userWallet.slice(-4)}`;
    document.getElementById("wallet").innerHTML = abridged;
    loadBalances();*/
}

async function connectWallet() {
    const ethereum = window.ethereum;
    const request_button = document.getElementById("request_button");

    let accounts;
    try {
        accounts = await ethereum.request({ method: "eth_requestAccounts" });
        request_button.removeEventListener("click", connectWallet);
    } catch (err) {
        console.log(err);
    }
}

function onLoad() {
    const request_button = document.getElementById("request_button");

    if (typeof window.ethereum == "undefined") {
        request_button.disabled = true;
        request_button.innerHTML = "Wallet provider not found";
        return;
    } else {
        let ethereum = window.ethereum;
        let accounts;

        ethereum
            .request({ method: "eth_accounts" })
            .then(walletChange)
            .catch((err) => {
                console.error(err)
            });

        ethereum.on('accountsChanged', walletChange);
    }
}

window.addEventListener("load", onLoad);
