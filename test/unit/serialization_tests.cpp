#define BOOST_TEST_MODULE serialization_tests
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>



#include <iostream>
#include <vector>
#include <fstream>


int argc = boost::unit_test::framework::master_test_suite().argc;
char ** argv = boost::unit_test::framework::master_test_suite().argv;






BOOST_AUTO_TEST_CASE( serialization_basic )
{
    std::string hello = "hello world";

    {

        std::ofstream os("serialize.cereal", std::ios::binary);

        cereal::PortableBinaryOutputArchive archive( os );


        archive( hello );
    }

    {

        std::string res;

        std::ifstream is("serialize.cereal", std::ios::binary);

        cereal::PortableBinaryInputArchive archive( is );


        archive( res );

        BOOST_CHECK_EQUAL(res, hello );

        std::cout << " recover " << res << std::endl;
    }


    {
        std::string buffer;
        std::vector<std::string> vec = { "hello", "world", "how", "are" , "you", "?" };

        {

            std::ostringstream oss(buffer, std::ios::binary);

            cereal::PortableBinaryOutputArchive archive( oss );

            archive( vec );

            buffer = oss.str();
        }

        BOOST_CHECK(buffer.size() > 0 );
        std::cout << "size buffer " << buffer.size() << " " << buffer << std::endl;

        {
            std::vector<std::string> res;

            std::istringstream iss(buffer, std::ios::binary);


            cereal::PortableBinaryInputArchive archive( iss );

            archive( res );

            BOOST_CHECK_EQUAL(res.size(), vec.size());
        }

    }

}
