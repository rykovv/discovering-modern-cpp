// oracle db includes

struct environment_deleter {
    void operator()( Environment* env )
    { Environment::terminateEnvironment(env); }
};

shared_ptr<Environment> environment(
    Environment::createEnvironment(), environment_deleter{});

// next resourve dependency solved

struct connection_deleter
{
    connection_deleter(shared_ptr<Environment> env)
      : env{env} {}
    void operator()(Connection* conn)
    { env->terminateConnection(conn); }
    shared_ptr<Environment> env;
};

shared_ptr<Connection> connection(environment->createConnection(...),
                                   connection_deleter{environment});

// single responsibility principle (SRP) applied
struct statement_deleter {
		statement_deleter(shared_ptr<Connection> conn)
			: conn{conn} {}
		void operator()( Statement *stmt ) {
				conn->terminateStatement(stmt);
		}
		shared_ptr<Connection> conn;
}

struct result_set_deleter {
		result_set_deleter(shared_ptr<Statement> statement)
			: statement{statement} {}
		void operator()( ResultSet *rs ) {
				statement->closeResultSet(rs);
		}
		shared_ptr<Statement> statement;
}

// modified version with SRP applied
class db_manager
{
  public:
    using ResultSetSharedPtr= std::shared_ptr<ResultSet>;

    db_manager(string const& dbConnection, string const& dbUser,
               string const& dbPw)
      : environment{Environment::createEnvironment(),
                    environment_deleter{}},
        connection{environment->createConnection(dbUser, dbPw,
                                                 dbConnection),
                   connection_deleter{environment} }
    {}
    // some getters ...
    ResultSetSharedPtr query(const std::string& q) const
    {
        shared_ptr<Statement> statement(connection->createStatement(q), 
																				statement_deleter{});
        ResultSet *rs = statement->executeQuery();
        auto rs_deleter = result_set_deleter{statement};

        return ResultSetSharedPtr{rs, rs_deleter};
    }
  private:
    shared_ptr<Environment> environment;
    shared_ptr<Connection>  connection;
};

int main()
{
    db_manager db("172.17.42.1", "herbert", "NSA_go_away");
    auto rs= db.query("select problem from my_solutions "
                        "   where award_worthy != 0");
    while (rs->next())
        cout << rs->getString(1) << endl;
}