#include <iostream>
#include <memory>
#include <vector>
#include <set>
#include <sstream>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>
#include <windows.h>

class Publisher;
class Stock;

class Book {
public:
	std::string									title{};
	Wt::Dbo::ptr<Publisher>						publisher{};
	Wt::Dbo::collection < Wt::Dbo::ptr<Stock>>	stocks;

	template <typename Action>
	void persist(Action& a) {
		Wt::Dbo::field(a, title, "title");
		Wt::Dbo::belongsTo(a, publisher, "publisher");
		Wt::Dbo::hasMany(a, stocks, Wt::Dbo::ManyToOne, "book");
	}
};

class Publisher {
public:
	std::string									name{};
	Wt::Dbo::collection<Wt::Dbo::ptr<Book>>		books{};

	template <typename Action>
	void persist(Action& a) {
		Wt::Dbo::field(a, name, "name");
		Wt::Dbo::hasMany(a, books, Wt::Dbo::ManyToOne, "publisher");
	}
};

class Shop {
public:
	std::string									name{};
	Wt::Dbo::collection <Wt::Dbo::ptr<Stock>>	stocks;

	template <typename Action>
	void persist(Action& a) {
		Wt::Dbo::field(a, name, "name");
		Wt::Dbo::hasMany(a, stocks, Wt::Dbo::ManyToOne, "shop");
	}
};

class Sale {
public:
	std::string									price{};
	std::string									data_sale{};
	Wt::Dbo::ptr<Stock>							stock{};
	int											count{};

	template <typename Action>
	void persist(Action& a) {
		Wt::Dbo::field(a, price, "price");
		Wt::Dbo::field(a, data_sale, "data sale");
		Wt::Dbo::belongsTo(a, stock, "stock");
		Wt::Dbo::field(a, count, "count");
	}
};

class Stock {
public:
	Wt::Dbo::ptr<Book>							book{};
	Wt::Dbo::ptr<Shop>							shop{};
	int											count{};
	Wt::Dbo::collection<Wt::Dbo::ptr<Sale>>		sales{};

	template <typename Action>
	void persist(Action& a) {
		Wt::Dbo::belongsTo(a, book, "book");
		Wt::Dbo::belongsTo(a, shop, "shop");
		Wt::Dbo::field(a, count, "count");
		Wt::Dbo::hasMany(a, sales, Wt::Dbo::ManyToOne, "stock");
	}
};

int main(int argc, char** argv) {

	std::srand(static_cast<unsigned>(std::time(0)));

	try {
		std::string connection_str =
			"host = localhost "
			"port = 5432 "
			"dbname = book_sales "
			"user = test_user "
			"password = test_password";

		auto postgres = std::make_unique<Wt::Dbo::backend::Postgres>(connection_str);

		Wt::Dbo::Session session;

		session.setConnection(std::move(postgres));

		session.mapClass<Publisher>("publisher");
		session.mapClass<Book>("book");
		session.mapClass<Shop>("shop");
		session.mapClass<Stock>("stock");
		session.mapClass<Sale>("sale");

		try
		{
			session.dropTables();
		}
		catch (const Wt::Dbo::Exception& ex) {
			std::cout << "There are no tables: " << ex.what() << std::endl;
		}

		session.createTables();

		Wt::Dbo::Transaction transaction{session};

		// PUBLISHERS
		std::vector<std::string> publisher_names { "АСТ", "Эскмо"};

		for (const auto& name : publisher_names) {
			auto publisher = std::make_unique<Publisher>();
			publisher->name = name;
			session.add(std::move(publisher));
		}

		// BOOKS
		std::vector<std::string>  book_titles {"1984", "Три товарища", "Рецидивист", "Алиса в стране чудес", "Война и мир"};

		for (const auto& title : book_titles) {
			auto book = std::make_unique<Book>();

			int index = std::rand() % publisher_names.size();

			Wt::Dbo::ptr<Publisher> p = session.find<Publisher>().where("name = ?").bind(publisher_names[index]);

			book->title = title;
			book->publisher = p;
			session.add(std::move(book));
		}

		// SHOPS
		std::vector<std::string> shop_names {"Читай город", "Лабиринт", "Литрес", "Республика", "Москвоский дом книги", "Букбридж"};

		for (const auto& name : shop_names) {
			auto shop = std::make_unique<Shop>();
			shop->name = name;
			session.add(std::move(shop));
		}

		// STOCK
		std::unique_ptr<Stock> stock;

		auto counter_stock = std::rand() % 11 + 10;

		for (size_t i = 0; i < counter_stock; i++) {
			stock = std::make_unique<Stock>();

			int index_b = std::rand() % book_titles.size();
			int index_sh = std::rand() % shop_names.size();

			Wt::Dbo::ptr<Book> b = session.find<Book>().where("title = ?").bind(book_titles[index_b]);
			Wt::Dbo::ptr<Shop> sh = session.find<Shop>().where("name = ?").bind(shop_names[index_sh]);

			stock->book = b;
			stock->shop = sh;
			stock->count = std::rand() % 16 + 15;

			session.add(std::move(stock));
		}

		Wt::Dbo::collection<Wt::Dbo::ptr<Stock>> stocks = session.find<Stock>();
		
		// SALE
		std::unique_ptr<Sale> sale;

		auto counter_sale = std::rand() % 5 + 1;

		for (size_t i = 0; i < counter_sale; i++) {
			sale = std::make_unique<Sale>();

			int index = std::rand() % stocks.size() + 1;
			Wt::Dbo::ptr<Stock> s = session.find<Stock>().where("id = ?").bind(index);

			sale->price = std::to_string(std::rand() % 5 + 1) + " $";

			// Generation year: 2020 - 2023
			int year = std::rand() % 4 + 2020;
			// Generation month: 1 - 12
			int month = std::rand() % 12 + 1;
			// Generation day: 1 - 20
			int day = std::rand() % 20 + 1;

			sale->data_sale = std::to_string(year) + "-" + std::to_string(month) + "-" + std::to_string(day);
			sale->stock = s;
			sale->count = std::rand() % 5 + 1;

			session.add(std::move(sale));
		}

		// USER INPUT
		std::string input_str{};
		int id_deal_publisher{};

		while (true) {
			std::cout << "Input publisher ID: ";

			std::getline(std::cin, input_str);

			std::stringstream iss(input_str);

			if (!(iss >> id_deal_publisher && iss.eof())) {
				std::cout << "Incorrect input. Try again..." << std::endl;
			}
			else if (id_deal_publisher > publisher_names.size()) {
				std::cout << "Publisher ID must not exceed " << publisher_names.size() << std::endl;
			}
			else break;
		}

		system("cls");

		std::set<std::string> shop_names_for_deal_publisher{};

		Wt::Dbo::ptr<Publisher> deal_publisher = session.find<Publisher>().where("name = ?").bind(publisher_names[id_deal_publisher - 1]);

		for (auto book : deal_publisher->books) {
			for (auto stock : book->stocks) {
				shop_names_for_deal_publisher.insert(stock->shop->name);
			}
		}

		std::cout << "Books of the publisher '" << deal_publisher->name << "' are sold in shops: " << std::endl;

		for (auto shop_name : shop_names_for_deal_publisher) {
			std::cout << " - " << shop_name << std::endl;
		}

		transaction.commit();
	}
	catch (const Wt::Dbo::Exception& ex) {
		std::cout << ex.what() << std::endl;
	}

	return 0;
}