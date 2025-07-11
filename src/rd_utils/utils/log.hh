#pragma once

#include <ctime>
#include <iostream>
#include <sstream>
#include <rd_utils/concurrency/mutex.hh>
#include <fstream>
#include <cstdint>

#ifndef __PROJECT__
#define __PROJECT__ "RD_UTILS"
#endif

#ifndef LOG_LEVEL
#define LOG_LEVEL 2
#endif

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_WARN 2
#define LOG_LEVEL_INFO 3
#define LOG_LEVEL_SUCCESS 4
#define LOG_LEVEL_DEBUG 5

namespace rd_utils::utils {

    enum class LogLevel : int {
		NONE = 0,
		ERROR,
		WARN,
		INFO,
		SUCCESS,
		STRANGE,
		DEBUG,
		ALL
    };


#define PURPLE "\e[1;35m"
#define BLUE "\e[1;36m"
#define YELLOW "\e[1;33m"
#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define BOLD "\e[1;50m"
#define UNDERLINE "\e[4m"
#define RESET  "\e[0m"

    /**
     * @returns: the current time encoded in string
     */
    std::string get_time ();

    /**
     * @returns: the current time encoded in string (attached with '_' instead of spaces)
     */
    std::string get_time_no_space ();

    /**
     * For variadic call (does nothing)
     */
    void content_print (std::ostream&);

    /**
     * write a in the stream s
     */
    template <typename T>
    void content_print (std::ostream& s, const T& a) {
		s << a;
    }

    /**
     * write a and b in the stream s
     */
    template <typename T, typename ... R>
    void content_print (std::ostream & s, const T& a, const R&... b) {
		s << a;
		content_print (s, b...);
    }

    
    class Logger {
    private :

		// the mutex log is static for multiple logger capability
		static concurrency::mutex __mutex__;

		// The descriptor of the output file
		std::ostream* _stream;

		// The file desc
		std::ofstream _file;

		// The ofstream path
		std::string _path;

	
		static Logger * __globalInstance__;
	
		// The level of logging
		LogLevel _level = LogLevel::ALL;

    private :

		Logger (const Logger & other);

		void operator=(const Logger& other);
	
    public : 

		/**
		 * Create a logger that write to logPath
		 * @params:
		 *     - logPath: the log file
		 *     - lvl: the level of logging
		 */
		Logger (const std::string & logPath, LogLevel lvl = LogLevel::SUCCESS) ;

		/**
		 * Create a logger that writes to stdout
		 * @params:
		 *    - lvl: the level of logging
		 */
		Logger (LogLevel lvl = LogLevel::SUCCESS);

	
		/**
		 * write into logPath from now on
		 * @warning: clear the file at logPath
		 */
		void redirect (const std::string& logPath, bool erase = false);

		/**
		 * Change the lvl of the logger
		 */
		void changeLevel (LogLevel lvl);

		/**
		 * change the lvl of the logger from the level name
		 */
		void changeLevel (const std::string & level);
	
		/**
		 * @returns: the log level of the logger
		 */
		std::string getLogLevel () const;
	
		/**
		 * @returns: the path of the log file (might be == "")
		 */
		const std::string & getLogFilePath () const;

		/**
		 * @returns: the global instance of the logger
		 */
		static Logger& globalInstance ();


	
		/**
		 * Info log (LogLevel >= LogLevel::INFO)
		 */
		template <typename ... T>
		void info (const char * file, const T&... msg) {
			if (this-> _level >= LogLevel::INFO) {
				WITH_LOCK (__mutex__) {
					(*this-> _stream) << "[" << BLUE << "INFO" << RESET << "][" << file << "][" << get_time ()  << "] ";
					content_print (*this-> _stream, msg...);
					(*this-> _stream) << std::endl;
				}
			}
		}

		/**
		 * Debug log (LogLevel >= LogLevel::DEBUG)
		 */
		template <typename ... T>
		void debug (const char* file, const T&... msg) {
			if (this-> _level >= LogLevel::DEBUG) {
				WITH_LOCK (__mutex__) {
					(*this-> _stream) << "[" << PURPLE << "DEBUG" << RESET << "][" << file << "][" << get_time ()  << "] ";
					content_print (*this-> _stream, msg...);
					(*this-> _stream) << std::endl;
				}
			}
		}
	
		/**
		 * Info log (LogLevel >= LogLevel::ERROR)
		 */
		template <typename ... T>
		void error (const char* file, const T&... msg) {
			if (this-> _level >= LogLevel::ERROR) {
				WITH_LOCK (__mutex__) {
					(*this-> _stream) << "[" << RED << "ERROR" << RESET << "][" << file << "][" << get_time () << "] ";
					content_print (*this-> _stream, msg...);
					(*this-> _stream) << std::endl;
					this-> _stream-> flush ();
				}
			}
		}
       
		/**
		 * Info log (LogLevel >= LogLevel::SUCCESS)
		 */
		template <typename ... T>
		void success (const char* file, const T&... msg) {
			if (this-> _level >= LogLevel::SUCCESS) {
				WITH_LOCK (__mutex__) {
					*this-> _stream << "[" << GREEN << "SUCCESS" << RESET << "][" << file << "][" << get_time ()  << "] ";
					content_print (*this-> _stream, msg...);
					*this-> _stream << std::endl;
					this-> _stream-> flush ();
				}
			}
		}

		/**
		 * Info log (LogLevel >= LogLevel::WARN)
		 */
		template <typename ... T>
		void warn (const char* file, T... msg) {
			if (this-> _level >= LogLevel::WARN) {
				WITH_LOCK (__mutex__) {
					*this-> _stream << "[" << YELLOW << "WARNING" << RESET << "][" << file << "][" << get_time ()  << "] ";
					content_print (*this-> _stream, msg...);
					*this-> _stream << std::endl;
					this-> _stream-> flush ();
				}
			}
		}
	
		/**
		 * Info log (LogLevel >= LogLevel::STRANGE)
		 */
		template <typename ... T>
		void strange (const char * file, const T&... msg) {
			if (this-> _level >= LogLevel::STRANGE) {
				WITH_LOCK (__mutex__) {
					*this-> _stream << "[" << PURPLE << "STRANGE" << RESET << "][ " << file << "][" << get_time ()  << "] ";
					content_print (*this-> _stream, msg...);
					*this-> _stream << std::endl;
					this-> _stream-> flush ();
				}
			}
		}

		/**
		 * Dispose the logger (write to stdout from now on)
		 */
		void dispose ();

		/**
		 * Dispose the global logger
		 */
		static void clear ();
	
		/**
		 * Call this-> dispose ()
		 */
		~Logger ();
	
	};
      	
}

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define LOG_INFO(...) rd_utils::utils::Logger::globalInstance ().info (__PROJECT__, __VA_ARGS__);
#else
#define LOG_INFO(...) {}
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define LOG_DEBUG(...) rd_utils::utils::Logger::globalInstance ().debug (__PROJECT__, __VA_ARGS__);
#else
#define LOG_DEBUG(...) {}
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define LOG_ERROR(...) rd_utils::utils::Logger::globalInstance ().error (__PROJECT__, __VA_ARGS__);
#else
#define LOG_ERROR(...) {}
#endif

#if LOG_LEVEL >= LOG_LEVEL_SUCCESS
#define LOG_SUCCESS(...) rd_utils::utils::Logger::globalInstance ().success (__PROJECT__, __VA_ARGS__);
#else
#define LOG_SUCCESS(...) {}
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define LOG_WARN(...) rd_utils::utils::Logger::globalInstance ().warn (__PROJECT__, __VA_ARGS__);
#else
#define LOG_WARN(...) {}
#endif

#define LOG_STRANGE(...) rd_utils::utils::Logger::globalInstance ().strange (__PROJECT__, __VA_ARGS__);
