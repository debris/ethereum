#pragma once

/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file KeyAux.cpp
 * @author Gav Wood <i@gavwood.com>
 * @date 2014
 * CLI module for key management.
 */

#include <thread>
#include <chrono>
#include <fstream>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <libdevcore/SHA3.h>
#include <libdevcore/FileSystem.h>
#include <libethcore/KeyManager.h>
#include <libethcore/ICAP.h>
#include <libethcore/Transaction.h>
#include <libdevcrypto/WordList.h>
#include "ConfigInfo.h"
using namespace std;
using namespace dev;
using namespace dev::eth;
using namespace boost::algorithm;

#undef RETURN

class BadArgument: public Exception {};

string getAccountPassword(KeyManager& keyManager, Address const& a)
{
	return getPassword("Enter password for address " + keyManager.accountName(a) + " (" + a.abridged() + "; hint:" + keyManager.passwordHint(a) + "): ");
}

string createPassword(std::string const& _prompt)
{
	string ret;
	while (true)
	{
		ret = getPassword(_prompt);
		string confirm = getPassword("Please confirm the password by entering it again: ");
		if (ret == confirm)
			break;
		cout << "Passwords were different. Try again." << endl;
	}
	return ret;
//	cout << "Enter a hint to help you remember this password: " << flush;
//	cin >> hint;
//	return make_pair(ret, hint);
}

pair<string, string> createPassword(KeyManager& _keyManager, std::string const& _prompt, std::string const& _pass = std::string(), std::string const& _hint = std::string())
{
	string pass = _pass;
	if (pass.empty())
		while (true)
		{
			pass = getPassword(_prompt);
			string confirm = getPassword("Please confirm the password by entering it again: ");
			if (pass == confirm)
				break;
			cout << "Passwords were different. Try again." << endl;
		}
	string hint = _hint;
	if (hint.empty() && !pass.empty() && !_keyManager.haveHint(pass))
	{
		cout << "Enter a hint to help you remember this password: " << flush;
		getline(cin, hint);
	}
	return make_pair(pass, hint);
}

class KeyCLI
{
public:
	enum class OperationMode
	{
		None,
		ListBare,
		NewBare,
		ImportBare,
		ExportBare,
		RecodeBare,
		KillBare,
		InspectBare,
		CreateWallet,
		List,
		New,
		Import,
		ImportWithAddress,
		ImportPresale,
		Export,
		Recode,
		Kill,
		NewBrain,
		ImportBrain,
		InspectBrain,
		SignTx,
		DecodeTx,
	};

	KeyCLI(OperationMode _mode = OperationMode::None): m_mode(_mode) {}

	bool interpretOption(int& i, int argc, char** argv)
	{
		string arg = argv[i];
		if (arg == "--wallet-path" && i + 1 < argc)
			m_walletPath = argv[++i];
		else if (arg == "--secrets-path" && i + 1 < argc)
			m_secretsPath = argv[++i];
		else if ((arg == "-m" || arg == "--master") && i + 1 < argc)
			m_masterPassword = argv[++i];
		else if (arg == "--unlock" && i + 1 < argc)
			m_unlocks.push_back(argv[++i]);
		else if (arg == "--lock" && i + 1 < argc)
			m_lock = argv[++i];
		else if (arg == "--kdf" && i + 1 < argc)
			m_kdf = argv[++i];
		else if (arg == "--kdf-param" && i + 2 < argc)
		{
			auto n = argv[++i];
			auto v = argv[++i];
			m_kdfParams[n] = v;
		}
		else if (arg == "--sign-tx" && i + 1 < argc)
		{
			m_mode = OperationMode::SignTx;
			m_signKey = argv[++i];
		}
		else if (arg == "--tx-data" && i + 1 < argc)
			try
			{
				m_toSign.data = fromHex(argv[++i]);
			}
			catch (...)
			{
				cerr << "Invalid argument to " << arg << endl;
				exit(-1);
			}
		else if (arg == "--tx-nonce" && i + 1 < argc)
			try
			{
				m_toSign.nonce = u256(argv[++i]);
			}
			catch (...)
			{
				cerr << "Invalid argument to " << arg << endl;
				exit(-1);
			}
		else if ((arg == "--tx-dest" || arg == "--tx-to" || arg == "--tx-destination") && i + 1 < argc)
			try
			{
				m_toSign.creation = false;
				m_toSign.to = toAddress(argv[++i]);
			}
			catch (...)
			{
				cerr << "Invalid argument to " << arg << endl;
				exit(-1);
			}
		else if (arg == "--tx-gas" && i + 1 < argc)
			try
			{
				m_toSign.gas = u256(argv[++i]);
			}
			catch (...)
			{
				cerr << "Invalid argument to " << arg << endl;
				exit(-1);
			}
		else if (arg == "--tx-gasprice" && i + 1 < argc)
			try
			{
				m_toSign.gasPrice = u256(argv[++i]);
			}
			catch (...)
			{
				cerr << "Invalid argument to " << arg << endl;
				exit(-1);
			}
		else if (arg == "--tx-value" && i + 1 < argc)
			try
			{
				m_toSign.value = u256(argv[++i]);
			}
			catch (...)
			{
				cerr << "Invalid argument to " << arg << endl;
				exit(-1);
			}
		else if (arg == "--decode-tx")
			m_mode = OperationMode::DecodeTx;
		else if (arg == "--import-bare")
			m_mode = OperationMode::ImportBare;
		else if (arg == "--list-bare")
			m_mode = OperationMode::ListBare;
		else if (arg == "--export-bare")
			m_mode = OperationMode::ExportBare;
		else if (arg == "--inspect-bare")
			m_mode = OperationMode::InspectBare;
		else if (arg == "--recode-bare")
			m_mode = OperationMode::RecodeBare;
		else if (arg == "--kill-bare")
			m_mode = OperationMode::KillBare;
		else if (arg == "--create-wallet")
			m_mode = OperationMode::CreateWallet;
		else if (arg == "-l" || arg == "--list")
			m_mode = OperationMode::List;
		else if ((arg == "-n" || arg == "--new") && i + 1 < argc)
		{
			m_mode = OperationMode::New;
			m_name = argv[++i];
		}
		else if ((arg == "-i" || arg == "--import") && i + 2 < argc)
		{
			m_mode = OperationMode::Import;
			m_inputs = strings(1, argv[++i]);
			m_name = argv[++i];
		}
		else if (arg == "--import-presale" && i + 2 < argc)
		{
			m_mode = OperationMode::ImportPresale;
			m_inputs = strings(1, argv[++i]);
			m_name = argv[++i];
		}
		else if (arg == "--new-brain" && i + 1 < argc)
		{
			m_mode = OperationMode::NewBrain;
			m_name = argv[++i];
		}
		else if (arg == "--import-brain" && i + 1 < argc)
		{
			m_mode = OperationMode::ImportBrain;
			m_name = argv[++i];
		}
		else if (arg == "--inspect-brain")
			m_mode = OperationMode::InspectBrain;
		else if (arg == "--import-with-address" && i + 3 < argc)
		{
			m_mode = OperationMode::ImportWithAddress;
			m_inputs = strings(1, argv[++i]);
			m_address = Address(argv[++i]);
			m_name = argv[++i];
		}
		else if (arg == "--export")
			m_mode = OperationMode::Export;
		else if (arg == "--recode")
			m_mode = OperationMode::Recode;
		else if (arg == "--no-icap")
			m_icap = false;
		else if (m_mode == OperationMode::DecodeTx || m_mode == OperationMode::SignTx || m_mode == OperationMode::ImportBare || m_mode == OperationMode::InspectBare || m_mode == OperationMode::KillBare || m_mode == OperationMode::Recode || m_mode == OperationMode::Export || m_mode == OperationMode::RecodeBare || m_mode == OperationMode::ExportBare)
			m_inputs.push_back(arg);
		else
			return false;
		return true;
	}

	KeyPair makeKey() const
	{
		KeyPair k(Secret::random());
		while (m_icap && k.address()[0])
			k = KeyPair(Secret(sha3(k.secret().ref())));
		return k;
	}

	Secret getSecret(std::string const& _signKey)
	{
		string json = contentsString(_signKey);
		if (!json.empty())
			return Secret(secretStore().secret(secretStore().readKeyContent(json), [&](){ return getPassword("Enter password for key: "); }));
		else
		{
			if (h128 u = fromUUID(_signKey))
				return Secret(secretStore().secret(u, [&](){ return getPassword("Enter password for key: "); }));
			if (_signKey.substr(0, 6) == "brain#" && _signKey.find(":") != string::npos)
				return KeyManager::subkey(KeyManager::brain(_signKey.substr(_signKey.find(":"))), stoul(_signKey.substr(6, _signKey.find(":") - 7)));
			if (_signKey.substr(0, 6) == "brain:")
				return KeyManager::brain(_signKey.substr(6));
			if (_signKey == "brain")
				return KeyManager::brain(getPassword("Enter brain wallet phrase: "));
			Address a;
			DEV_IGNORE_EXCEPTIONS(a = Address(_signKey));
			if (a)
				return keyManager().secret(a, [&](){ return getPassword("Enter password for key (hint:" + keyManager().passwordHint(a) + "): "); });
			cerr << "Bad file, UUID and address: " << _signKey << endl;
			exit(-1);
		}
	}

	void execute()
	{
		switch (m_mode)
		{
		case OperationMode::CreateWallet:
		{
			KeyManager wallet(m_walletPath, m_secretsPath);
			if (m_masterPassword.empty())
				m_masterPassword = createPassword("Please enter a MASTER password to protect your key store (make it strong!): ");
			if (m_masterPassword.empty())
				cerr << "Aborted (empty password not allowed)." << endl;
			else
			{
				try
				{
					wallet.create(m_masterPassword);
				}
				catch (Exception const& _e)
				{
					cerr << "unable to create wallet" << endl << boost::diagnostic_information(_e);
				}
			}
			break;
		}
		case OperationMode::DecodeTx:
		{
			bytes b = inputData(m_inputs[0]);
			if (b.empty())
				cerr << "Unknown file or bad hex: '" << m_inputs[0] << "'" << endl;
			else
				try
				{
					TransactionBase t(b, CheckTransaction::None);
					cout << "Transaction " << t.sha3().hex() << endl;
					if (t.isCreation())
					{
						cout << "  type: creation" << endl;
						cout << "  code: " << toHex(t.data()) << endl;
					}
					else
					{
						cout << "  type: message" << endl;
						cout << "  to: " << t.to().hex() << endl;
						cout << "  data: " << (t.data().empty() ? "none" : toHex(t.data())) << endl;
					}
					try
					{
						auto s = t.sender();
						if (t.isCreation())
							cout << "  creates: " << toAddress(s, t.nonce()).hex() << endl;
						cout << "  from: " << s.hex() << endl;
					}
					catch (...)
					{
						cout << "  from: <unsigned>" << endl;
					}
					cout << "  value: " << formatBalance(t.value()) << " (" << t.value() << " wei)" << endl;
					cout << "  nonce: " << t.nonce() << endl;
					cout << "  gas: " << t.gas() << endl;
					cout << "  gas price: " << formatBalance(t.gasPrice()) << " (" << t.gasPrice() << " wei)" << endl;
					cout << "  signing hash: " << t.sha3(WithoutSignature).hex() << endl;
					cout << "  v: " << (int)t.signature().v << endl;
					cout << "  r: " << t.signature().r << endl;
					cout << "  s: " << t.signature().s << endl;
				}
				catch (Exception& ex)
				{
					cerr << "Invalid transaction: " << ex.what() << endl;
				}
			break;
		}
		case OperationMode::SignTx:
		{
			Secret s = getSecret(m_signKey);
			if (m_inputs.empty())
				m_inputs.push_back(string());
			for (string const& i: m_inputs)
			{
				bool isFile = false;
				try
				{
					TransactionBase t = i.empty() ? TransactionBase(m_toSign) : TransactionBase(inputData(i, &isFile), CheckTransaction::None);
					t.sign(s);
					cout << t.sha3() << ": ";
					if (isFile)
					{
						writeFile(i + ".signed", toHex(t.rlp()));
						cout << i + ".signed" << endl;
					}
					else
						cout << toHex(t.rlp()) << endl;
				}
				catch (Exception& ex)
				{
					cerr << "Invalid transaction: " << ex.what() << endl;
				}
			}
			break;
		}
		case OperationMode::NewBrain:
		{
			if (m_name != "--")
				keyManager();
			boost::random_device d;
			boost::random::uniform_int_distribution<unsigned> pickWord(0, WordList.size() - 1);
			string seed;
			for (int i = 0; i < 13; ++i)
				seed += (seed.size() ? " " : "") + std::string{WordList[pickWord(d)]};
			cout << "Your brain key phrase: <<" << seed << ">>" << endl;
			if (m_name != "--")
			{
				std::string hint;
				cout << "Enter a hint for the phrase if you want: " << flush;
				getline(cin, hint);
				Address a = keyManager().importBrain(seed, m_name, hint);
				cout << a.abridged() << endl;
				cout << "  ICAP: " << ICAP(a).encoded() << endl;
				cout << "  Address: " << a.hex() << endl;
			}
			break;
		}
		case OperationMode::InspectBrain:
		{
			Address a = toAddress(KeyManager::brain(getPassword("Enter brain wallet key phrase: ")));
			cout << a.abridged() << endl;
			cout << "  ICAP: " << ICAP(a).encoded() << endl;
			cout << "  Address: " << a.hex() << endl;
			break;
		}
		case OperationMode::ListBare:
			for (h128 const& u: std::set<h128>() + secretStore().keys())
				cout << toUUID(u) << endl;
			break;
		case OperationMode::NewBare:
		{
			if (m_lock.empty())
				m_lock = createPassword("Enter a password with which to secure this account: ");
			auto k = makeKey();
			h128 u = secretStore().importSecret(k.secret().ref(), m_lock);
			cout << "Created key " << toUUID(u) << endl;
			cout << "  Address: " << k.address().hex() << endl;
			cout << "  ICAP: " << ICAP(k.address()).encoded() << endl;
			break;
		}
		case OperationMode::ImportBare:
			for (string const& input: m_inputs)
			{
				h128 u;
				bytesSec b;
				b.writable() = fromHex(input);
				if (b.size() != 32)
				{
					std::string s = contentsString(input);
					b.writable() = fromHex(s);
					if (b.size() != 32)
						u = secretStore().importKey(input);
				}
				if (!u && b.size() == 32)
					u = secretStore().importSecret(b, lockPassword(toAddress(Secret(b)).abridged()));
				if (!u)
				{
					cerr << "Cannot import " << input << " not a file or secret." << endl;
					continue;
				}
				cout << "Successfully imported " << input << " as " << toUUID(u);
			}
			break;
		case OperationMode::InspectBare:
			for (auto const& i: m_inputs)
				if (!contents(i).empty())
				{
					h128 u = secretStore().readKey(i, false);
					bytesSec s = secretStore().secret(u, [&](){ return getPassword("Enter password for key " + i + ": "); });
					cout << "Key " << i << ":" << endl;
					cout << "  UUID: " << toUUID(u) << ":" << endl;
					cout << "  Address: " << toAddress(Secret(s)).hex() << endl;
					cout << "  Secret: " << toHex(s.ref().cropped(0, 8)) << "..." << endl;
				}
				else if (h128 u = fromUUID(i))
				{
					bytesSec s = secretStore().secret(u, [&](){ return getPassword("Enter password for key " + toUUID(u) + ": "); });
					cout << "Key " << i << ":" << endl;
					cout << "  Address: " << toAddress(Secret(s)).hex() << endl;
					cout << "  Secret: " << toHex(s.ref().cropped(0, 8)) << "..." << endl;
				}
				else
					cerr << "Couldn't inspect " << i << "; not found." << endl;
			break;
		case OperationMode::ExportBare: break;
		case OperationMode::RecodeBare:
			for (auto const& i: m_inputs)
				if (h128 u = fromUUID(i))
					if (secretStore().recode(u, lockPassword(toUUID(u)), [&](){ return getPassword("Enter password for key " + toUUID(u) + ": "); }, kdf()))
						cerr << "Re-encoded " << toUUID(u) << endl;
					else
						cerr << "Couldn't re-encode " << toUUID(u) << "; key corrupt or incorrect password supplied." << endl;
				else
					cerr << "Couldn't re-encode " << i << "; not found." << endl;
			break;
		case OperationMode::KillBare:
			for (auto const& i: m_inputs)
				if (h128 u = fromUUID(i))
					secretStore().kill(u);
				else
					cerr << "Couldn't kill " << i << "; not found." << endl;
			break;
		case OperationMode::New:
		{
			keyManager();
			tie(m_lock, m_lockHint) = createPassword(keyManager(), "Enter a password with which to secure this account (or nothing to use the master password): ", m_lock, m_lockHint);
			auto k = makeKey();
			bool usesMaster = m_lock.empty();
			h128 u = usesMaster ? keyManager().import(k.secret(), m_name) : keyManager().import(k.secret(), m_name, m_lock, m_lockHint);
			cout << "Created key " << toUUID(u) << endl;
			cout << "  Name: " << m_name << endl;
			if (usesMaster)
				cout << "  Uses master password." << endl;
			else
				cout << "  Password hint: " << m_lockHint << endl;
			cout << "  Address: " << k.address().hex() << endl;
			cout << "  ICAP: " << ICAP(k.address()).encoded() << endl;
			break;
		}
		case OperationMode::ImportWithAddress:
		{
			keyManager();
			string const& i = m_inputs[0];
			h128 u;
			bytesSec b;
			b.writable() = fromHex(i);
			if (b.size() != 32)
			{
				std::string s = contentsString(i);
				b.writable() = fromHex(s);
				if (b.size() != 32)
					u = keyManager().store().importKey(i);
			}
			if (!u && b.size() == 32)
				u = keyManager().store().importSecret(b, lockPassword(toAddress(Secret(b)).abridged()));
			if (!u)
			{
				cerr << "Cannot import " << i << " not a file or secret." << endl;
				break;
			}
			keyManager().importExisting(u, m_name, m_address);
			cout << "Successfully imported " << i << ":" << endl;
			cout << "  Name: " << m_name << endl;
			cout << "  Address: " << m_address << endl;
			cout << "  UUID: " << toUUID(u) << endl;
			break;
		}
		case OperationMode::ImportBrain:
		{
			keyManager();
			std::string seed = getPassword("Enter brain wallet key phrase: ");
			std::string hint;
			cout << "Enter a hint for the phrase if you want: " << flush;
			getline(cin, hint);
			Address a = keyManager().importBrain(seed, m_name, hint);
			cout << a << endl;
			cout << "  ICAP: " << ICAP(a).encoded() << endl;
			cout << "  Address: " << a.hex() << endl;
			break;
		}
		case OperationMode::ImportPresale:
		{
			keyManager();
			std::string pw;
			KeyPair k = keyManager().presaleSecret(contentsString(m_inputs[0]), [&](bool){ return (pw = getPassword("Enter the password for the presale key: ")); });
			keyManager().import(k.secret(), m_name, pw, "Same password as used for presale key");
			break;
		}
		case OperationMode::List:
		{
			vector<u128> bare;
			AddressHash got;

			for (auto const& u: keyManager().store().keys())
				if (Address a = keyManager().address(u))
				{
					got.insert(a);
					cout << toUUID(u) << " " << a.abridged();
					string s = ICAP(a).encoded();
					cout << " " << s << string(35 - s.size(), ' ');
					cout << " " << keyManager().accountName(a) << endl;
				}
				else
					bare.push_back(u);
			for (auto const& a: keyManager().accounts())
				if (!got.count(a))
				{
					cout << "               (Brain)               " << a.abridged();
					cout << " " << ICAP(a).encoded() << " ";
					cout << " " << keyManager().accountName(a) << endl;
				}
			for (auto const& u: bare)
				cout << toUUID(u) << " (Bare)" << endl;
		}
		default: break;
		}
	}

	std::string lockPassword(std::string const& _accountName)
	{
		return m_lock.empty() ? createPassword("Enter a password with which to secure account " + _accountName + ": ") : m_lock;
	}

	static void streamHelp(ostream& _out)
	{
		_out
			<< "Secret-store (\"bare\") operation modes:" << endl
			<< "    --list-bare  List all secret available in secret-store." << endl
			<< "    --new-bare  Generate and output a key without interacting with wallet and dump the JSON." << endl
			<< "    --import-bare [ <file>|<secret-hex> , ... ] Import keys from given sources." << endl
			<< "    --recode-bare [ <uuid>|<file> , ... ]  Decrypt and re-encrypt given keys." << endl
			<< "    --inspect-bare [ <uuid>|<file> , ... ]  Output information on given keys." << endl
//			<< "    --export-bare [ <uuid> , ... ]  Export given keys." << endl
			<< "    --kill-bare [ <uuid> , ... ]  Delete given keys." << endl
			<< "Secret-store configuration:" << endl
			<< "    --secrets-path <path>  Specify Web3 secret-store path (default: " << SecretStore::defaultPath() << ")" << endl
			<< endl
			<< "Wallet operating modes:" << endl
			<< "    -l,--list  List all keys available in wallet." << endl
			<< "    -n,--new <name>  Create a new key with given name and add it in the wallet." << endl
			<< "    -i,--import [<uuid>|<file>|<secret-hex>] <name>  Import keys from given source and place in wallet." << endl
			<< "    --import-presale <file> <name>  Import a presale wallet into a key with the given name." << endl
			<< "    --import-with-address [<uuid>|<file>|<secret-hex>] <address> <name>  Import keys from given source with given address and place in wallet." << endl
			<< "    -e,--export [ <address>|<uuid> , ... ]  Export given keys." << endl
			<< "    -r,--recode [ <address>|<uuid>|<file> , ... ]  Decrypt and re-encrypt given keys." << endl
			<< "Brain wallet operating modes:" << endl
			<< "WARNING: Brain wallets with human-generated passphrasses are highly susceptible to attack. Don't use such a thing for" << endl
			<< "anything important." << endl
			<< "    --new-brain [ <name>|-- ]  Create a new 13-word brain wallet; argument is the name or if --, do not add to wallet."<< endl
			<< "    --import-brain <name>  Import your own brain wallet." << endl
			<< "    --inspect-brain  Check the address of a particular brain wallet." << endl
			<< "Wallet configuration:" << endl
			<< "    --create-wallet  Create an Ethereum master wallet." << endl
			<< "    --wallet-path <path>  Specify Ethereum wallet path (default: " << KeyManager::defaultPath() << ")" << endl
			<< "    -m, --master <password>  Specify wallet (master) password." << endl
			<< endl
			<< "Transaction operating modes:" << endl
			<< "    -d,--decode-tx [<hex>|<file>]  Decode given transaction." << endl
			<< "    -s,--sign-tx [ <address>|<uuid>|<file>|brain((#<HD-index>):<brain-phrase>) ] [ <hex>|<file> , ... ]  (Re-)Sign given transaction." << endl
			<< endl
			<< "Encryption configuration:" << endl
			<< "    --kdf <kdfname>  Specify KDF to use when encrypting (default: sc	rypt)" << endl
			<< "    --kdf-param <name> <value>  Specify a parameter for the KDF." << endl
//			<< "    --cipher <ciphername>  Specify cipher to use when encrypting (default: aes-128-ctr)" << endl
//			<< "    --cipher-param <name> <value>  Specify a parameter for the cipher." << endl
			<< "    --lock <password>  Specify password for when encrypting a (the) key." << endl
			<< "    --hint <hint>  Specify hint for the --lock password." << endl
			<< endl
			<< "Decryption configuration:" << endl
			<< "    --unlock <password>  Specify password for a (the) key." << endl
			<< "Key generation configuration:" << endl
			<< "    --no-icap  Don't bother to make a direct-ICAP capable key." << endl
			;
	}

	static bytes inputData(std::string const& _input, bool* _isFile = nullptr)
	{
		bytes b = fromHex(_input);
		if (_isFile)
			*_isFile = false;
		if (b.empty())
		{
			if (_isFile)
				*_isFile = true;
			std::string s = boost::trim_copy_if(contentsString(_input), is_any_of(" \t\n"));
			b = fromHex(s);
			if (b.empty())
				b = asBytes(s);
		}
		return b;
	}

	static bool isTrue(std::string const& _m)
	{
		return _m == "on" || _m == "yes" || _m == "true" || _m == "1";
	}

	static bool isFalse(std::string const& _m)
	{
		return _m == "off" || _m == "no" || _m == "false" || _m == "0";
	}

private:
	void openWallet(KeyManager& _w)
	{
		while (true)
		{
			if (_w.load(m_masterPassword))
				break;
			if (!m_masterPassword.empty())
			{
				cout << "Password invalid. Try again." << endl;
				m_masterPassword.clear();
			}
			m_masterPassword = getPassword("Please enter your MASTER password: ");
		}
	}

	KDF kdf() const { return m_kdf == "pbkdf2" ? KDF::PBKDF2_SHA256 : KDF::Scrypt; }

	KeyManager& keyManager()
	{
		if (!m_keyManager)
		{
			m_keyManager.reset(new KeyManager(m_walletPath, m_secretsPath));
			if (m_keyManager->exists())
				openWallet(*m_keyManager);
			else
			{
				cerr << "Couldn't open wallet. Does it exist?" << endl;
				exit(-1);
			}
		}
		return *m_keyManager;
	}

	SecretStore& secretStore()
	{
		if (m_keyManager)
			return m_keyManager->store();
		if (!m_secretStore)
			m_secretStore.reset(new SecretStore(m_secretsPath));
		return *m_secretStore;
	}

	/// Where the keys are.
	unique_ptr<SecretStore> m_secretStore;
	unique_ptr<KeyManager> m_keyManager;

	/// Operating mode.
	OperationMode m_mode;

	/// Wallet stuff
	string m_secretsPath = SecretStore::defaultPath();
	string m_walletPath = KeyManager::defaultPath();

	/// Wallet password stuff
	string m_masterPassword;
	strings m_unlocks;
	string m_lock;
	string m_lockHint;
	bool m_icap = true;

	/// Creating/importing
	string m_name;
	Address m_address;

	/// Importing
	strings m_inputs;

	/// Signing
	string m_signKey;
	TransactionSkeleton m_toSign;

	string m_kdf = "scrypt";
	map<string, string> m_kdfParams;
//	string m_cipher;
//	map<string, string> m_cipherParams;
};
