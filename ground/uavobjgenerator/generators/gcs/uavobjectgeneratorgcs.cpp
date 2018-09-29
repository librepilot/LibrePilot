/**
 ******************************************************************************
 *
 * @file       uavobjectgeneratorgcs.cpp
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2016.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      produce gcs code for uavobjects
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <QSet>
#include "uavobjectgeneratorgcs.h"

#define VERBOSE             false
#define DEPRECATED          true

#define DEFAULT_ENUM_PREFIX "E_"

using namespace std;

void error(QString msg)
{
    cerr << "error: " << msg.toStdString() << endl;
}

void warning(ObjectInfo *object, QString msg)
{
    cerr << "warning: " << object->filename.toStdString() << ": " << msg.toStdString() << endl;
}

void info(ObjectInfo *object, QString msg)
{
    if (VERBOSE) {
        cout << "info: " << object->filename.toStdString() << ": " << msg.toStdString() << endl;
    }
}

struct Context {
    ObjectInfo    *object;
    // parent objects
    QSet<QString> parentObjects;
    // enums
    QString enums;
    QString enumsCount;
    QString registerImpl;
    // interface
    QString fields;
    QString fieldsInfo;
    QString properties;
    QString deprecatedProperties;
    QString getters;
    QString setters;
    QString notifications;
    // implementation
    QString fieldsInit;
    QString fieldsDefault;
    QString propertiesImpl;
    QString notificationsImpl;
};

struct FieldContext {
    FieldContext(FieldInfo *fieldInfo, ObjectInfo *object);
    FieldContext(const FieldContext &fieldContext);

    FieldInfo *field;
    // field
    QString   fieldName;
    QString   fieldType;
    // property
    QString   propName;
    QString   ucPropName;
    QString   propType;
    QString   propRefType;
    //
    QString   parentClassName;
    QString   parentFieldName;
    QString   ucParentPropName;
    // deprecation
    bool hasDeprecatedProperty;
    bool hasDeprecatedGetter;
    bool hasDeprecatedSetter;
    bool hasDeprecatedNotification;
};

QString fieldTypeStrCPP(int type)
{
    QStringList fieldTypeStrCPP;

    fieldTypeStrCPP << "qint8" << "qint16" << "qint32" << "quint8" << "quint16" << "quint32" << "float" << "quint8";
    return fieldTypeStrCPP[type];
}

QString fieldTypeStrCPPClass(int type)
{
    QStringList fieldTypeStrCPPClass;

    fieldTypeStrCPPClass << "INT8" << "INT16" << "INT32" << "UINT8" << "UINT16" << "UINT32" << "FLOAT32" << "ENUM";
    return fieldTypeStrCPPClass[type];
}

QString toPropertyName(const QString & name)
{
    QString str = name;

    // make sure 1st letter is upper case
    str[0] = str[0].toUpper();

    // handle underscore
    int p = str.indexOf('_');
    while (p != -1) {
        str.remove(p, 1);
        str[p] = str[p].toUpper();
        p = str.indexOf('_', p);
    }

    return str;
}

/*
 * Convert a string to lower camel case.
 * Handles following cases :
 * - Property -> property
 * - MyProperty -> myProperty
 * - MYProperty -> myProperty
 * - MY_Property -> my_Property
 * - MY -> my
 */
QString toLowerCamelCase(const QString & name)
{
    QString str = name;

    for (int i = 0; i < str.length(); ++i) {
        if (str[i].isLower() || !str[i].isLetter()) {
            break;
        }
        if (i > 0 && i < str.length() - 1) {
            // after first, look ahead one
            if (str[i + 1].isLower()) {
                break;
            }
        }
        str[i] = str[i].toLower();
    }

    return str;
}

QString toEnumName(ObjectInfo *object, FieldInfo *field, int index)
{
    QString option = field->options[index];

    if (option.contains(QRegExp(ENUM_SPECIAL_CHARS))) {
        info(object, "Enumeration value \"" + option + "\" contains special chars, cleaning.");
        option.replace(QRegExp(ENUM_SPECIAL_CHARS), "");
    }
    if (option[0].isDigit()) {
        info(object, "Enumeration value \"" + option + "\" starts with a digit, prefixing with \"" + DEFAULT_ENUM_PREFIX "\".");
        option = DEFAULT_ENUM_PREFIX + option;
    }
    if (option == option.toLower()) {
        warning(object, "Enumeration value \"" + option + "\" is all lower case, consider capitalizing.");
    }
    if (option[0].isLower()) {
        warning(object, "Enumeration value \"" + option + "\" does not start with an upper case letter.");
        option[0] = option[0].toUpper();
    }
    if (option == "FALSE") {
        warning(object, "Invalid enumeration name FALSE, converting to False.");
        option = "False";
    }
    if (option == "TRUE") {
        warning(object, "Invalid enumeration name TRUE, converting to True.");
        option = "True";
    }
    return option;
}

QString toEnumStringList(ObjectInfo *object, FieldInfo *field)
{
    QString enumListString;

    for (int m = 0; m < field->options.length(); ++m) {
        if (m > 0) {
            enumListString.append(", ");
        }
        QString option = toEnumName(object, field, m);
        enumListString.append(option);
    }

    return enumListString;
}


QString generate(Context &ctxt, const QString &fragment)
{
    QString str = fragment;

    str.replace(":ClassName", ctxt.object->name);
    str.replace(":className", ctxt.object->namelc);

    return str;
}

QString generate(Context &ctxt, FieldContext &fieldCtxt, const QString &fragment)
{
    QString str = generate(ctxt, fragment);

    str.replace(":PropName", fieldCtxt.ucPropName);
    str.replace(":propName", fieldCtxt.propName);
    str.replace(":propType", fieldCtxt.propType);
    str.replace(":propRefType", fieldCtxt.propRefType);

    str.replace(":fieldName", fieldCtxt.fieldName);
    str.replace(":fieldType", fieldCtxt.fieldType);
    str.replace(":fieldDesc", fieldCtxt.field->description);
    str.replace(":fieldUnits", fieldCtxt.field->units);
    str.replace(":fieldLimitValues", fieldCtxt.field->limitValues);

    str.replace(":elementCount", QString::number(fieldCtxt.field->numElements));
    str.replace(":enumCount", QString::number(fieldCtxt.field->numOptions));

    str.replace(":parentFieldName", fieldCtxt.parentFieldName);
    str.replace(":parentClassName", fieldCtxt.parentClassName);
    str.replace(":ParentPropName", fieldCtxt.ucParentPropName);

    return str;
}

void generateFieldInfo(Context &ctxt, FieldContext &fieldCtxt)
{
    ctxt.fieldsInfo += generate(ctxt, fieldCtxt, "    // :fieldName\n");

    if (fieldCtxt.field->type == FIELDTYPE_ENUM) {
        if (fieldCtxt.field->parentObjectName.length() > 0) {
            ctxt.fieldsInfo += generate(ctxt, fieldCtxt, "    typedef :parentClassName:::parentFieldNameOptions :fieldNameOptions;\n");
        } else {
            QStringList options = fieldCtxt.field->options;
            ctxt.fieldsInfo += "    typedef enum { ";
            for (int m = 0; m < options.length(); ++m) {
                if (m > 0) {
                    ctxt.fieldsInfo.append(", ");
                }
                ctxt.fieldsInfo += generate(ctxt, fieldCtxt, "%1_%2=%3")
                                   .arg(fieldCtxt.field->name.toUpper())
                                   .arg(options[m].toUpper().replace(QRegExp(ENUM_SPECIAL_CHARS), ""))
                                   .arg(m);
            }
            ctxt.fieldsInfo += generate(ctxt, fieldCtxt, " } :fieldNameOptions;\n");
        }
    }

    // Generate element names (only if field has more than one element)
    if (fieldCtxt.field->numElements > 1 && !fieldCtxt.field->defaultElementNames) {
        if (fieldCtxt.field->parentObjectName.length() > 0) {
            ctxt.fieldsInfo += generate(ctxt, fieldCtxt, "    typedef :parentClassName:::parentFieldNameElem :fieldNameElem;\n");
        } else {
            QStringList elemNames = fieldCtxt.field->elementNames;
            ctxt.fieldsInfo += "    typedef enum { ";
            for (int m = 0; m < elemNames.length(); ++m) {
                if (m > 0) {
                    ctxt.fieldsInfo.append(", ");
                }
                ctxt.fieldsInfo += QString("%1_%2=%3")
                                   .arg(fieldCtxt.field->name.toUpper())
                                   .arg(elemNames[m].toUpper())
                                   .arg(m);
            }
            ctxt.fieldsInfo += generate(ctxt, fieldCtxt, " } :fieldNameElem;\n");
        }
    }

    // Generate array information
    if (fieldCtxt.field->numElements > 1) {
        ctxt.fieldsInfo += generate(ctxt, fieldCtxt, "    static const quint32 %1_NUMELEM = :elementCount;\n")
                           .arg(fieldCtxt.field->name.toUpper());
    }
}

void generateFieldInit(Context &ctxt, FieldContext &fieldCtxt)
{
    ctxt.fieldsInit += generate(ctxt, fieldCtxt, "    // :fieldName\n");

    // Setup element names
    ctxt.fieldsInit += generate(ctxt, fieldCtxt, "    QStringList :fieldNameElemNames;\n");
    QStringList elemNames = fieldCtxt.field->elementNames;
    ctxt.fieldsInit += generate(ctxt, fieldCtxt, "    :fieldNameElemNames");
    for (int m = 0; m < elemNames.length(); ++m) {
        ctxt.fieldsInit += QString(" << \"%1\"").arg(elemNames[m]);
    }
    ctxt.fieldsInit += ";\n";

    if (fieldCtxt.field->type == FIELDTYPE_ENUM) {
        ctxt.fieldsInit += generate(ctxt, fieldCtxt, "    QStringList :fieldNameEnumOptions;\n");
        QStringList options = fieldCtxt.field->options;
        ctxt.fieldsInit += generate(ctxt, fieldCtxt, "    :fieldNameEnumOptions");
        for (int m = 0; m < options.length(); ++m) {
            ctxt.fieldsInit += QString(" << \"%1\"").arg(options[m]);
        }
        ctxt.fieldsInit += ";\n";
        ctxt.fieldsInit += generate(ctxt, fieldCtxt,
                                    "    fields.append(new UAVObjectField(\":fieldName\", tr(\":fieldDesc\"), \":fieldUnits\", UAVObjectField::ENUM, :fieldNameElemNames, :fieldNameEnumOptions, \":fieldLimitValues\"));\n");
    } else {
        ctxt.fieldsInit += generate(ctxt, fieldCtxt,
                                    "    fields.append(new UAVObjectField(\":fieldName\", tr(\":fieldDesc\"), \":fieldUnits\", UAVObjectField::%1, :fieldNameElemNames, QStringList(), \":fieldLimitValues\"));\n")
                           .arg(fieldTypeStrCPPClass(fieldCtxt.field->type));
    }
}

void generateFieldDefault(Context &ctxt, FieldContext &fieldCtxt)
{
    if (!fieldCtxt.field->defaultValues.isEmpty()) {
        // For non-array fields
        if (fieldCtxt.field->numElements == 1) {
            ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    // :fieldName\n");
            if (fieldCtxt.field->type == FIELDTYPE_ENUM) {
                ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    data_.:fieldName = %1;\n")
                                      .arg(fieldCtxt.field->options.indexOf(fieldCtxt.field->defaultValues[0]));
            } else if (fieldCtxt.field->type == FIELDTYPE_FLOAT32) {
                ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    data_.:fieldName = %1;\n")
                                      .arg(fieldCtxt.field->defaultValues[0].toFloat());
            } else {
                ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    data_.:fieldName = %1;\n")
                                      .arg(fieldCtxt.field->defaultValues[0].toInt());
            }
        } else {
            // Initialize all fields in the array
            ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    // :fieldName\n");
            for (int idx = 0; idx < fieldCtxt.field->numElements; ++idx) {
                if (fieldCtxt.field->type == FIELDTYPE_ENUM) {
                    ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    data_.:fieldName[%1] = %2;\n")
                                          .arg(idx)
                                          .arg(fieldCtxt.field->options.indexOf(fieldCtxt.field->defaultValues[idx]));
                } else if (fieldCtxt.field->type == FIELDTYPE_FLOAT32) {
                    ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    data_.:fieldName[%1] = %2;\n")
                                          .arg(idx)
                                          .arg(fieldCtxt.field->defaultValues[idx].toFloat());
                } else {
                    ctxt.fieldsDefault += generate(ctxt, fieldCtxt, "    data_.:fieldName[%1] = %2;\n")
                                          .arg(idx)
                                          .arg(fieldCtxt.field->defaultValues[idx].toInt());
                }
            }
        }
    }
}

void generateField(Context &ctxt, FieldContext &fieldCtxt)
{
    if (fieldCtxt.parentClassName.length() > 0) {
        ctxt.parentObjects.insert(fieldCtxt.parentClassName);
    }
    if (fieldCtxt.field->numElements > 1) {
        ctxt.fields += generate(ctxt, fieldCtxt, "        :fieldType :fieldName[:elementCount];\n");
    } else {
        ctxt.fields += generate(ctxt, fieldCtxt, "        :fieldType :fieldName;\n");
    }
    generateFieldInfo(ctxt, fieldCtxt);
    generateFieldInit(ctxt, fieldCtxt);
    generateFieldDefault(ctxt, fieldCtxt);
}

void generateEnum(Context &ctxt, FieldContext &fieldCtxt)
{
    Q_ASSERT(fieldCtxt.field->type == FIELDTYPE_ENUM);

    if (fieldCtxt.field->parentObjectName.length() > 0) {
        ctxt.enums += generate(ctxt, fieldCtxt, "typedef :parentClassName_:ParentPropName :ClassName_:PropName;\n\n");
    } else {
        QString enumStringList = toEnumStringList(ctxt.object, fieldCtxt.field);

        ctxt.enums += generate(ctxt, fieldCtxt,
                               "class :ClassName_:PropName : public QObject {\n"
                               "    Q_OBJECT\n"
                               "public:\n"
                               "    enum Enum { %1 };\n"
                               "    Q_ENUMS(Enum) // TODO switch to Q_ENUM once on Qt 5.5\n"
                               "};\n\n").arg(enumStringList);

        ctxt.enumsCount   += generate(ctxt, fieldCtxt, ":PropNameCount = :enumCount, ");

        ctxt.registerImpl += generate(ctxt, fieldCtxt,
                                      "    qmlRegisterType<:ClassName_:PropName>(\"%1.:ClassName\", 1, 0, \":PropName\");\n").arg("UAVTalk");
    }
}

void generateBaseProperty(Context &ctxt, FieldContext &fieldCtxt)
{
    ctxt.properties        += generate(ctxt, fieldCtxt,
                                       "    Q_PROPERTY(:propType :propName READ :propName WRITE set:PropName NOTIFY :propNameChanged)\n");

    ctxt.getters           += generate(ctxt, fieldCtxt, "    :propType :propName() const;\n");
    ctxt.setters           += generate(ctxt, fieldCtxt, "    void set:PropName(const :propRefType value);\n");

    ctxt.notifications     += generate(ctxt, fieldCtxt, "    void :propNameChanged(const :propRefType value);\n");
    ctxt.notificationsImpl += generate(ctxt, fieldCtxt, "    emit :propNameChanged(:propName());\n");

    if (DEPRECATED) {
        // generate deprecated property for retro compatibility
        if (fieldCtxt.hasDeprecatedProperty) {
            ctxt.deprecatedProperties += generate(ctxt, fieldCtxt,
                                                  "    /*DEPRECATED*/ Q_PROPERTY(:fieldType :fieldName READ get:fieldName WRITE set:fieldName NOTIFY :fieldNameChanged);\n");
        }
        if (fieldCtxt.hasDeprecatedGetter) {
            ctxt.getters += generate(ctxt, fieldCtxt,
                                     "    /*DEPRECATED*/ Q_INVOKABLE :fieldType get:fieldName() const { return static_cast<:fieldType>(:propName()); }\n");
        }
        if (fieldCtxt.hasDeprecatedSetter) {
            ctxt.setters += generate(ctxt, fieldCtxt,
                                     "    /*DEPRECATED*/ void set:fieldName(:fieldType value) { set:PropName(static_cast<:propType>(value)); }\n");
        }
        if (fieldCtxt.hasDeprecatedNotification) {
            ctxt.notifications     += generate(ctxt, fieldCtxt,
                                               "    /*DEPRECATED*/ void :fieldNameChanged(:fieldType value);\n");

            ctxt.notificationsImpl += generate(ctxt, fieldCtxt,
                                               "    /*DEPRECATED*/ emit :fieldNameChanged(get:fieldName());\n");
        }
    }
}

void generateSimpleProperty(Context &ctxt, FieldContext &fieldCtxt)
{
    if (fieldCtxt.field->type == FIELDTYPE_ENUM) {
        generateEnum(ctxt, fieldCtxt);
    }

    generateBaseProperty(ctxt, fieldCtxt);

    // getter implementation
    ctxt.propertiesImpl += generate(ctxt, fieldCtxt,
                                    ":propType :ClassName:::propName() const\n"
                                    "{\n"
                                    "   QMutexLocker locker(mutex);\n"
                                    "   return static_cast<:propType>(data_.:fieldName);\n"
                                    "}\n");

    // emitters
    QString emitters = generate(ctxt, fieldCtxt, "emit :propNameChanged(value);");

    if (fieldCtxt.hasDeprecatedNotification) {
        emitters += " ";
        emitters += generate(ctxt, fieldCtxt, "emit :fieldNameChanged(static_cast<:fieldType>(value));");
    }

    // setter implementation
    ctxt.propertiesImpl += generate(ctxt, fieldCtxt,
                                    "void :ClassName::set:PropName(const :propRefType value)\n"
                                    "{\n"
                                    "   mutex->lock();\n"
                                    "   bool changed = (data_.:fieldName != static_cast<:fieldType>(value));\n"
                                    "   data_.:fieldName = static_cast<:fieldType>(value);\n"
                                    "   mutex->unlock();\n"
                                    "   if (changed) { %1 }\n"
                                    "}\n\n").arg(emitters);
}

void generateIndexedProperty(Context &ctxt, FieldContext &fieldCtxt)
{
    if (fieldCtxt.field->type == FIELDTYPE_ENUM) {
        generateEnum(ctxt, fieldCtxt);
    }

    // indexed getter/setter
    ctxt.getters += generate(ctxt, fieldCtxt, "    Q_INVOKABLE :propType :propName(quint32 index) const;\n");
    ctxt.setters += generate(ctxt, fieldCtxt, "    Q_INVOKABLE void set:PropName(quint32 index, const :propRefType value);\n");

    // getter implementation
    ctxt.propertiesImpl += generate(ctxt, fieldCtxt,
                                    ":propType :ClassName:::propName(quint32 index) const\n"
                                    "{\n"
                                    "   QMutexLocker locker(mutex);\n"
                                    "   return static_cast<:propType>(data_.:fieldName[index]);\n"
                                    "}\n");

    // emitters
    QString emitters = generate(ctxt, fieldCtxt, "emit :propNameChanged(index, value);");

    if (fieldCtxt.hasDeprecatedNotification) {
        emitters += " ";
        emitters += generate(ctxt, fieldCtxt, "emit :fieldNameChanged(index, static_cast<:fieldType>(value));");
    }

    // setter implementation
    ctxt.propertiesImpl += generate(ctxt, fieldCtxt,
                                    "void :ClassName::set:PropName(quint32 index, const :propRefType value)\n"
                                    "{\n"
                                    "   mutex->lock();\n"
                                    "   bool changed = (data_.:fieldName[index] != static_cast<:fieldType>(value));\n"
                                    "   data_.:fieldName[index] = static_cast<:fieldType>(value);\n"
                                    "   mutex->unlock();\n"
                                    "   if (changed) { %1 }\n"
                                    "}\n\n").arg(emitters);

    ctxt.notifications += generate(ctxt, fieldCtxt, "    void :propNameChanged(quint32 index, const :propRefType value);\n");

    if (DEPRECATED) {
        // backward compatibility
        if (fieldCtxt.hasDeprecatedGetter) {
            ctxt.getters += generate(ctxt, fieldCtxt,
                                     "    /*DEPRECATED*/ Q_INVOKABLE :fieldType get:fieldName(quint32 index) const { return static_cast<:fieldType>(:propName(index)); }\n");
        }
        if (fieldCtxt.hasDeprecatedSetter) {
            ctxt.setters += generate(ctxt, fieldCtxt,
                                     "    /*DEPRECATED*/ void set:fieldName(quint32 index, :fieldType value) { set:PropName(index, static_cast<:propType>(value)); }\n");
        }

        if (fieldCtxt.hasDeprecatedNotification) {
            ctxt.notifications += generate(ctxt, fieldCtxt,
                                           "    /*DEPRECATED*/ void :fieldNameChanged(quint32 index, :fieldType value);\n");
        }
    }

    for (int elementIndex = 0; elementIndex < fieldCtxt.field->numElements; elementIndex++) {
        QString elementName = fieldCtxt.field->elementNames[elementIndex];

        QString sep;
        if (fieldCtxt.propName.right(1)[0].isDigit() && elementName[0].isDigit()) {
            info(ctxt.object, "Property \"" + fieldCtxt.propName + "\" and element \"" + elementName + "\" have digit conflict, consider fixing it.");
            sep = "_";
        }

        FieldContext elementCtxt(fieldCtxt);
        elementCtxt.fieldName  = fieldCtxt.fieldName + "_" + elementName;
        elementCtxt.propName   = fieldCtxt.propName + sep + elementName;
        elementCtxt.ucPropName = fieldCtxt.ucPropName + sep + elementName;
        // deprecation
        elementCtxt.hasDeprecatedProperty     = (elementCtxt.fieldName != elementCtxt.propName) && DEPRECATED;
        elementCtxt.hasDeprecatedGetter       = DEPRECATED;
        elementCtxt.hasDeprecatedSetter       = ((elementCtxt.fieldName != elementCtxt.ucPropName) || (elementCtxt.fieldType != elementCtxt.propType)) && DEPRECATED;
        elementCtxt.hasDeprecatedNotification = ((elementCtxt.fieldName != elementCtxt.propName) || (elementCtxt.fieldType != elementCtxt.propType)) && DEPRECATED;


        generateBaseProperty(ctxt, elementCtxt);

        ctxt.propertiesImpl += generate(ctxt, elementCtxt,
                                        ":propType :ClassName:::propName() const { return %1(%2); }\n").arg(fieldCtxt.propName).arg(elementIndex);

        ctxt.propertiesImpl += generate(ctxt, elementCtxt,
                                        "void :ClassName::set:PropName(const :propRefType value) { set%1(%2, value); }\n").arg(fieldCtxt.ucPropName).arg(elementIndex);
    }
}

void generateProperty(Context &ctxt, FieldContext &fieldCtxt)
{
    // do some checks
    QString fieldName = fieldCtxt.fieldName;

    if (fieldName[0].isLower()) {
        info(ctxt.object, "Field \"" + fieldName + "\" does not start with an upper case letter.");
    }

    // generate all properties
    if (fieldCtxt.field->numElements > 1) {
        generateIndexedProperty(ctxt, fieldCtxt);
    } else {
        generateSimpleProperty(ctxt, fieldCtxt);
    }
}

bool UAVObjectGeneratorGCS::generate(UAVObjectParser *parser, QString templatepath, QString outputpath)
{
    gcsCodePath        = QDir(templatepath + QString(GCS_CODE_DIR));
    gcsOutputPath      = QDir(outputpath);
    gcsOutputPath.mkpath(gcsOutputPath.absolutePath());

    gcsCodeTemplate    = readFile(gcsCodePath.absoluteFilePath("uavobject.cpp.template"));
    gcsIncludeTemplate = readFile(gcsCodePath.absoluteFilePath("uavobject.h.template"));
    QString gcsInitTemplate = readFile(gcsCodePath.absoluteFilePath("uavobjectsinit.cpp.template"));

    if (gcsCodeTemplate.isEmpty() || gcsIncludeTemplate.isEmpty() || gcsInitTemplate.isEmpty()) {
        error("Error: Failed to read gcs code templates.");
        return false;
    }

    QString objInc;
    QString gcsObjInit;

    for (int objidx = 0; objidx < parser->getNumObjects(); ++objidx) {
        ObjectInfo *object = parser->getObjectByIndex(objidx);
        process_object(object);

        Context ctxt;
        ctxt.object = object;

        objInc.append(QString("#include \"%1.h\"\n").arg(object->namelc));

        gcsObjInit += ::generate(ctxt, "    objMngr->registerObject( new :ClassName() );\n");
        gcsObjInit += ::generate(ctxt, "    :ClassName::registerQMLTypes();\n");
    }

    // Write the gcs object initialization files
    gcsInitTemplate.replace("$(OBJINC)", objInc);
    gcsInitTemplate.replace("$(OBJINIT)", gcsObjInit);

    bool res = writeFileIfDifferent(gcsOutputPath.absolutePath() + "/uavobjectsinit.cpp", gcsInitTemplate);
    if (!res) {
        error("Error: Could not write output files");
        return false;
    }

    return true;
}

FieldContext::FieldContext(const FieldContext &fieldContext)
{
    *this = fieldContext;
}

FieldContext::FieldContext(FieldInfo *fieldInfo, ObjectInfo *object)
{
    field     = fieldInfo;

    // field properties
    fieldName = field->name;
    fieldType = fieldTypeStrCPP(field->type);

    parentClassName  = field->parentObjectName;
    parentFieldName  = field->parentFieldName;
    ucParentPropName = toPropertyName(field->parentFieldName);

    ucPropName = toPropertyName(field->name);
    propName   = toLowerCamelCase(ucPropName);
    propType   = fieldType;
    if (field->type == FIELDTYPE_INT8) {
        propType = fieldTypeStrCPP(FIELDTYPE_INT16);
    } else if (field->type == FIELDTYPE_UINT8) {
        propType = fieldTypeStrCPP(FIELDTYPE_UINT16);
    } else if (field->type == FIELDTYPE_ENUM) {
        QString enumClassName = object->name + "_" + ucPropName;
        propType = enumClassName + "::Enum";
    }
    // reference type
    propRefType = propType;

    // deprecation
    hasDeprecatedProperty     = (fieldName != propName) && DEPRECATED;
    hasDeprecatedGetter       = DEPRECATED;
    hasDeprecatedSetter       = ((fieldName != ucPropName) || (fieldType != propType)) && DEPRECATED;
    hasDeprecatedNotification = ((fieldName != propName) || (fieldType != propType)) && DEPRECATED;
}

/**
 * Generate the GCS object files
 *
 * TODO add getter to get enum names
 *
 * TODO handle "char" unit
 * TODO handle "bool" unit
 * TODO handle "hex" unit
 * TODO handle Vector
 */
bool UAVObjectGeneratorGCS::process_object(ObjectInfo *object)
{
    if (object == NULL) {
        return false;
    }

    // Prepare output strings
    QString outInclude = gcsIncludeTemplate;
    QString outCode    = gcsCodeTemplate;

    // Replace common tags
    replaceCommonTags(outInclude, object);
    replaceCommonTags(outCode, object);

    // to avoid name conflicts
    QStringList reservedProperties;
    reservedProperties << "Description" << "Metadata";

    Context ctxt;
    ctxt.object = object;

    ctxt.registerImpl += ::generate(ctxt,
                                    "    qmlRegisterType<:ClassName>(\"%1.:ClassName\", 1, 0, \":ClassName\");\n").arg("UAVTalk");

    ctxt.registerImpl += ::generate(ctxt,
                                    "    qmlRegisterType<:ClassNameConstants>(\"%1.:ClassName\", 1, 0, \":ClassNameConstants\");\n").arg("UAVTalk");

    for (int n = 0; n < object->fields.length(); ++n) {
        FieldInfo *field = object->fields[n];

        // field context
        FieldContext fieldCtxt(field, object);

        generateField(ctxt, fieldCtxt);

        if (reservedProperties.contains(field->name)) {
            warning(object, "Ignoring reserved property " + field->name + ".");
            continue;
        }

        generateProperty(ctxt, fieldCtxt);
    }

    QString includes;
    foreach(const QString &objectName, ctxt.parentObjects) {
        includes.append("#include \"" + objectName.toLower() + ".h\"\n");
    }

    outInclude.replace("$(INCLUDE)", includes);
    outInclude.replace("$(ENUMS)", ctxt.enums);
    outInclude.replace("$(ENUMS_COUNT)", ctxt.enumsCount);
    outInclude.replace("$(DATAFIELDS)", ctxt.fields);
    outInclude.replace("$(DATAFIELDINFO)", ctxt.fieldsInfo);
    outInclude.replace("$(PROPERTIES)", ctxt.properties);
    outInclude.replace("$(DEPRECATED_PROPERTIES)", ctxt.deprecatedProperties);
    outInclude.replace("$(PROPERTY_GETTERS)", ctxt.getters);
    outInclude.replace("$(PROPERTY_SETTERS)", ctxt.setters);
    outInclude.replace("$(PROPERTY_NOTIFICATIONS)", ctxt.notifications);

    outCode.replace("$(FIELDSINIT)", ctxt.fieldsInit);
    outCode.replace("$(FIELDSDEFAULT)", ctxt.fieldsDefault);
    outCode.replace("$(PROPERTIES_IMPL)", ctxt.propertiesImpl);
    outCode.replace("$(NOTIFY_PROPERTIES_CHANGED)", ctxt.notificationsImpl);
    outCode.replace("$(REGISTER_QML_TYPES)", ctxt.registerImpl);

    // Write the GCS code
    bool res = writeFileIfDifferent(gcsOutputPath.absolutePath() + "/" + object->namelc + ".cpp", outCode);
    if (!res) {
        error("Error: Could not write gcs output files");
        return false;
    }
    res = writeFileIfDifferent(gcsOutputPath.absolutePath() + "/" + object->namelc + ".h", outInclude);
    if (!res) {
        error("Error: Could not write gcs output files");
        return false;
    }

    return true;
}
