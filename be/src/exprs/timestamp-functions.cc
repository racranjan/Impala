// Copyright (c) 2011 Cloudera, Inc. All rights reserved.

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/time_zone_base.hpp>
#include <boost/date_time/local_time/local_time.hpp>

#include "exprs/timestamp-functions.h"
#include "exprs/expr.h"
#include "runtime/tuple-row.h"
#include "runtime/timestamp-value.h"
#include "util/path-builder.h"

#define TIMEZONE_DATABASE "be/files/date_time_zonespec.csv"

using namespace boost;
using namespace boost::posix_time;
using namespace boost::local_time;
using namespace boost::gregorian;
using namespace std;

namespace impala {

local_time::tz_database TimezoneDatabase::tz_database_;
vector<string> TimezoneDatabase::tz_region_list_;

// Implementation of UNIX_TIMESTAMP
//   int unix_timestamp(timestamp input)
// Just returns the internal representation. Not sure why this is different than
// cast to int.
void* TimestampFunctions::Unix(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  ptime temp;
  tv->ToPtime(&temp);
  e->result_.int_val = static_cast<int32_t>(to_time_t(temp));
  return &e->result_.int_val;
}

void* TimestampFunctions::Year(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  // If the value has been set to not_a_date_time then it will be marked special
  // therefore there is no valid date component and this function returns NULL.
  if (tv->date().is_special()) return NULL;
  e->result_.int_val = tv->date().year();
  return &e->result_.int_val;
}

void* TimestampFunctions::Month(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->date().is_special()) return NULL;
  e->result_.int_val = tv->date().month();
  return &e->result_.int_val;
}

void* TimestampFunctions::Day(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->date().is_special()) return NULL;
  e->result_.int_val = tv->date().day_of_year();
  return &e->result_.int_val;
}

void* TimestampFunctions::DayOfMonth(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->date().is_special()) return NULL;
  e->result_.int_val = tv->date().day();
  return &e->result_.int_val;
}

void* TimestampFunctions::WeekOfYear(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->date().is_special()) return NULL;
  e->result_.int_val = tv->date().week_number();
  return &e->result_.int_val;
}

void* TimestampFunctions::Hour(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->time_of_day().is_special()) return NULL;

  e->result_.int_val = tv->time_of_day().hours();
  return &e->result_.int_val;
}

void* TimestampFunctions::Minute(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->time_of_day().is_special()) return NULL;

  e->result_.int_val = tv->time_of_day().minutes();
  return &e->result_.int_val;
}

void* TimestampFunctions::Second(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  if (tv->time_of_day().is_special()) return NULL;

  e->result_.int_val = tv->time_of_day().seconds();
  return &e->result_.int_val;
}

void* TimestampFunctions::Now(Expr* e, TupleRow* row) {
  // Make sure FunctionCall::Prepare() properly set the timestamp value.
  DCHECK(!e->result_.timestamp_val.date().is_special());
  return &e->result_.timestamp_val;
}

void* TimestampFunctions::ToDate(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 1);
  Expr* op = e->children()[0];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op->GetValue(row));
  if (tv == NULL) return NULL;

  string result = to_iso_extended_string(tv->date());
  e->result_.SetStringVal(result);
  return &e->result_.string_val;
}

void* TimestampFunctions::YearsAdd(Expr* e, TupleRow* row) {
  return TimestampDateOp<years>(e, row, true);
}

void* TimestampFunctions::YearsSub(Expr* e, TupleRow* row) {
  return TimestampDateOp<years>(e, row, false);
}

void* TimestampFunctions::MonthsAdd(Expr* e, TupleRow* row) {
  return TimestampDateOp<months>(e, row, true);
}

void* TimestampFunctions::MonthsSub(Expr* e, TupleRow* row) {
  return TimestampDateOp<months>(e, row, false);
}

void* TimestampFunctions::WeeksAdd(Expr* e, TupleRow* row) {
  return TimestampDateOp<weeks>(e, row, true);
}

void* TimestampFunctions::WeeksSub(Expr* e, TupleRow* row) {
  return TimestampDateOp<weeks>(e, row, false);
}

void* TimestampFunctions::DaysAdd(Expr* e, TupleRow* row) {
  return TimestampDateOp<date_duration>(e, row, true);
}

void* TimestampFunctions::DaysSub(Expr* e, TupleRow* row) {
  return TimestampDateOp<date_duration>(e, row, false);
}

void* TimestampFunctions::HoursAdd(Expr* e, TupleRow* row) {
  return TimestampTimeOp<hours>(e, row, true);
}

void* TimestampFunctions::HoursSub(Expr* e, TupleRow* row) {
  return TimestampTimeOp<hours>(e, row, false);
}

void* TimestampFunctions::MinutesAdd(Expr* e, TupleRow* row) {
  return TimestampTimeOp<minutes>(e, row, true);
}

void* TimestampFunctions::MinutesSub(Expr* e, TupleRow* row) {
  return TimestampTimeOp<minutes>(e, row, false);
}

void* TimestampFunctions::SecondsAdd(Expr* e, TupleRow* row) {
  return TimestampTimeOp<seconds>(e, row, true);
}

void* TimestampFunctions::SecondsSub(Expr* e, TupleRow* row) {
  return TimestampTimeOp<seconds>(e, row, false);
}

void* TimestampFunctions::MillisAdd(Expr* e, TupleRow* row) {
  return TimestampTimeOp<milliseconds>(e, row, true);
}

void* TimestampFunctions::MillisSub(Expr* e, TupleRow* row) {
  return TimestampTimeOp<milliseconds>(e, row, false);
}

void* TimestampFunctions::MicrosAdd(Expr* e, TupleRow* row) {
  return TimestampTimeOp<microseconds>(e, row, true);
}

void* TimestampFunctions::MicrosSub(Expr* e, TupleRow* row) {
  return TimestampTimeOp<microseconds>(e, row, false);
}

void* TimestampFunctions::NanosAdd(Expr* e, TupleRow* row) {
  return TimestampTimeOp<nanoseconds>(e, row, true);
}

void* TimestampFunctions::NanosSub(Expr* e, TupleRow* row) {
  return TimestampTimeOp<nanoseconds>(e, row, false);
}

template <class UNIT>
void* TimestampFunctions::TimestampDateOp(Expr* e, TupleRow* row, bool is_add) {
  DCHECK_EQ(e->GetNumChildren(), 2);
  Expr* op1 = e->children()[0];
  Expr* op2 = e->children()[1];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op1->GetValue(row));
  int32_t* count = reinterpret_cast<int32_t*>(op2->GetValue(row));
  if (tv == NULL || count == NULL) return NULL;

  if (tv->date().is_special()) return NULL;

  UNIT unit(*count);
  TimestampValue
      value((is_add ? tv->date() + unit : tv->date() - unit), tv->time_of_day());
  e->result_.timestamp_val = value;

  return &e->result_.timestamp_val;
}

template <class UNIT>
void* TimestampFunctions::TimestampTimeOp(Expr* e, TupleRow* row, bool is_add) {
  DCHECK_EQ(e->GetNumChildren(), 2);
  Expr* op1 = e->children()[0];
  Expr* op2 = e->children()[1];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op1->GetValue(row));
  int32_t* count = reinterpret_cast<int32_t*>(op2->GetValue(row));
  if (tv == NULL || count == NULL) return NULL;

  if (tv->date().is_special()) return NULL;

  UNIT unit(*count);
  ptime p(tv->date(), tv->time_of_day());
  TimestampValue value(is_add ? p + unit : p - unit);
  e->result_.timestamp_val = value;

  return &e->result_.timestamp_val;
}

void* TimestampFunctions::DateDiff(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 2);
  Expr* op1 = e->children()[0];
  Expr* op2 = e->children()[1];
  TimestampValue* tv1 = reinterpret_cast<TimestampValue*>(op1->GetValue(row));
  TimestampValue* tv2 = reinterpret_cast<TimestampValue*>(op2->GetValue(row));
  if (tv1 == NULL || tv2 == NULL) return NULL;

  if (tv1->date().is_special()) return NULL;
  if (tv2->date().is_special()) return NULL;

  e->result_.int_val = (tv2->date() - tv1->date()).days();
  return &e->result_.int_val;
}

void* TimestampFunctions::FromUtc(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 2);
  Expr* op1 = e->children()[0];
  Expr* op2 = e->children()[1];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op1->GetValue(row));
  StringValue* tz = reinterpret_cast<StringValue*>(op2->GetValue(row));
  if (tv == NULL || tz == NULL) return NULL;

  if (tv->NotADateTime()) return NULL;

  time_zone_ptr timezone = TimezoneDatabase::FindTimezone(tz->DebugString());
  // This should raise some sort of error or at least null. Hive just ignores it.
  if (timezone == NULL) {
    LOG(ERROR) << "Unknown timezone '" << *tz << "'" << endl;
    e->result_.timestamp_val = *tv;
    return &e->result_.timestamp_val;
  }
  ptime temp;
  tv->ToPtime(&temp);
  local_date_time lt(temp, timezone);
  e->result_.timestamp_val = lt.local_time();
  return &e->result_.timestamp_val;
}

void* TimestampFunctions::ToUtc(Expr* e, TupleRow* row) {
  DCHECK_EQ(e->GetNumChildren(), 2);
  Expr* op1 = e->children()[0];
  Expr* op2 = e->children()[1];
  TimestampValue* tv = reinterpret_cast<TimestampValue*>(op1->GetValue(row));
  StringValue* tz = reinterpret_cast<StringValue*>(op2->GetValue(row));
  if (tv == NULL || tz == NULL) return NULL;

  if (tv->NotADateTime()) return NULL;

  time_zone_ptr timezone = TimezoneDatabase::FindTimezone(tz->DebugString());
  // This should raise some sort of error or at least null. Hive just ignores it.
  if (timezone == NULL) {
    LOG(ERROR) << "Unknown timezone '" << *tz << "'" << endl;
    e->result_.timestamp_val = *tv;
    return &e->result_.timestamp_val;
  }
  local_date_time lt(tv->date(), tv->time_of_day(),
                     timezone, local_date_time::NOT_DATE_TIME_ON_ERROR);
  e->result_.timestamp_val = TimestampValue(lt.utc_time());
  return &e->result_.timestamp_val;
}

TimezoneDatabase::TimezoneDatabase() {
  // Create a temporary file and write the timezone information.  The boost
  // interface only loads this format from a file.  We don't want to raise
  // an error here since this is done when the backend is created and this
  // information might not actually get used by any queries.
  char filestr[] = "/tmp/impala.tzdb.XXXXXXX";
  FILE* file;
  int fd;
  if ((fd = mkstemp(filestr)) == -1) {
    LOG(ERROR) << "Could not create temporary timezone file: " << filestr;
    return;
  }
  if ((file = fopen(filestr, "w")) == NULL) {
    unlink(filestr);
    close(fd);
    LOG(ERROR) << "Could not open temporary timezone file: " << filestr;
    return;
  }
  if (fputs(TIMEZONE_DATABASE_STR, file) == EOF) {
    unlink(filestr);
    close(fd);
    fclose(file);
    LOG(ERROR) << "Could not load temporary timezone file: " << filestr;
    return;
  }
  fclose(file);
  tz_database_.load_from_file(string(filestr));
  tz_region_list_ = tz_database_.region_list();
  unlink(filestr);
  close(fd);
}

TimezoneDatabase::~TimezoneDatabase() { }

time_zone_ptr TimezoneDatabase::FindTimezone(const string& tz) {
  // See if they specified a zone id
  if (tz.find_first_of('/') != string::npos)
    return  tz_database_.time_zone_from_region(tz);
  for (vector<string>::const_iterator iter = tz_region_list_.begin();
       iter != tz_region_list_.end(); ++iter) {
    time_zone_ptr tzp = tz_database_.time_zone_from_region(*iter);
    DCHECK(tzp != NULL);
    if (tzp->dst_zone_abbrev() == tz)
      return tzp;
    if (tzp->std_zone_abbrev() == tz)
      return tzp;
    if (tzp->dst_zone_name() == tz)
      return tzp;
    if (tzp->std_zone_name() == tz)
      return tzp;
  }
  return time_zone_ptr();

}

}
