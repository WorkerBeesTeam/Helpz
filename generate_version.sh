#!/bin/bash

OUT_PATH=$1
NS=$2
MAJ=$3
MIN=$4

if [ -z "$OUT_PATH" ] || [ -z "$NS" ] || [ -z "$MAJ" ] || [ -z "$MIN" ]; then
  echo "Usage: $0 /output/path namespace major_version minor_version"
  exit 1
fi

BUILD_FILE=${OUT_PATH}/build_number
[ -f $BUILD_FILE ] && BUILD=`cat $BUILD_FILE`
[[ $BUILD =~ ^[0-9]+$ ]] || BUILD=0
BUILD=$((${BUILD}+1))
echo "$BUILD" > $BUILD_FILE

OUT_FILE=${OUT_PATH}/version.h

H_DEF="GENERATED_VERSION_FILE_${NS^^}_H"

cat <<EOF > $OUT_FILE
#ifndef $H_DEF
#define $H_DEF

#include <QString>

namespace $NS {
namespace Version {

  static const unsigned short MAJOR = ${MAJ};
  static const unsigned short MINOR = ${MIN};
  static const unsigned short BUILD = ${BUILD};

  static QString getVersionString() {
    return QString("%1.%2.%3").arg(MAJOR).arg(MINOR).arg(BUILD);
  }

} // namespace Version
} // namespace $NS

#endif // $H_DEF
EOF

exit 0
