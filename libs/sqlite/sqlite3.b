#!-- part of the sqlite module

import .exception { * }
import .cursor { * }
import _sqlite { 
  _open, 
  _close, 
  _exec, 
  _last_insert_id, 
  _query 
}

/**
 * @class SQLite
 * 
 * SQLite3 class
 */
class SQLite3 {

  /**
   * the path to the SQLite3 file
   * @default = :memory:
   */
  var path = ':memory:'

  /**
   * pointer to sqlite3 C struct
   */
  var _db

  /**
   * tracks the open/closed state of the sqlite
   */
  var _is_open = false

  /**
   * @constructor SQLite3
   * 
   * SQLite3(path: string)
   * @note the database doesn't need to exist.
   */
  SQLite3(path) {
    if path != nil {
      if !is_string(path)
        die SQLiteException('database path expected')

      self.path = path
    }
  }

  /**
   * open()
   * 
   * opens the handle to a database file 
   */
  open() {
    self._db = _open(self.path)

    if is_string(self._db)
      die SQLiteException('could not open database: ${self._db}')

    self._is_open = true
  }

  /**
   * close()
   * 
   * closes the handle to the database and return `true` if successfully
   * closed or `false` otherwise.
   * @return boolean
   */
  close() {
    if(_close(self._db)) {
      self._is_open = false
      return true
    }

    return false
  }

  /**
   * exec(query: string)
   * 
   * executes a query string without and returns `true` if the
   * query was executed or `false` otherwise.
   * 
   * @note this method does not return a query result
   * @return boolean
   */
  exec(query) {
    if !is_string(query)
      die SQLiteException('string expected, ${typeof(query)} given')

    if !self._is_open
      die SQLiteException('database not open for exec')

    var result = _exec(self._db, query)
    if is_string(result)
      die SQLiteException('SQL error ${result}')

    return result
  }

  /**
   * last_insert_id()
   * 
   * the id of the last insert operation.
   * returns: 
   * * -1 if the last insert failed, 
   * * 0 if no insert statement has been executed or 
   * * a number greater than 0 if it succeeded
   * @returns number
   */
  last_insert_id() {

    if !self._is_open
      die SQLiteException('database not open for exec')

    return _last_insert_id(self._db)
  }

  /**
   * query(sql: string [, params: list | dict])
   * 
   * executes and sql query and returns the result of the execution
   * @note pass a list as _params_ if you have unnamed parameterized queries.
   * 
   * For example,
   * 
   * sqlite.query('SELECT * FROM users WHERE id = ? AND name = ?', [3, 'James'])
   * 
   * @note pass a dictionary as _params_ if you use named paramters
   * 
   * For example,
   * 
   * sqlite.query(
   *   'SELECT * FROM user WHERE id = :id AND name = :name', 
   *   {':id': 1, ':name': 'James'}
   * )
   */
  query(sql, params) {
    if params != nil {
      if !is_list(params) and !is_dict(params)
        die SQLiteException('list of query parameters expected')
    }

    if !self._is_open
      die SQLiteException('database not open for query')

    var result = _query(self._db, sql, params)
    if is_string(result) 
      die SQLiteException('SQL error ${result}')
    
      # if no error occurs, the _query function always returns a valid cursor
    return SQLite3Cursor(self, result)
  }

  /**
   * fetch(sql: string [, params: list | dict])
   * 
   * runs an SQL query and returns the result as a list of dictionaries.
   * 
   * @note if the result is empty or the query is not a SELECT, 
   * it returns an empty list
   */
  fetch(sql, params) {
    var cursor = self.query(sql, params)
    var result = []
    if cursor.columns.length() > 0 {
      while cursor.has_next() {
        var entry = {}
        for col in cursor.columns {
          entry.add(col, cursor.get(col))
        }
        result.append(entry)
      }
    }
    return result
  }
}

