/**
 * Copyright (C) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *   http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * @file	nntrainer_test_util.h
 * @date	28 April 2020
 * @brief	This is util functions for test
 * @see		https://github.com/nnstreamer/nntrainer
 * @author	Jijoong Moon <jijoong.moon@samsung.com>
 * @bug		No known bugs except for NYI items
 *
 */

#ifndef __NNTRAINER_TEST_UTIL_H__
#define __NNTRAINER_TEST_UTIL_H__
#ifdef __cplusplus

#include <fstream>
#include <unordered_map>

#include <neuralnet.h>
#include <nntrainer_error.h>
#include <nntrainer_log.h>
#include <parse_util.h>
#include <tensor.h>

#define tolerance 10e-5

/** Enum values to get model accuracy and loss. Sync with internal CAPI header
 */
#define ML_TRAIN_SUMMARY_MODEL_TRAIN_LOSS 101
#define ML_TRAIN_SUMMARY_MODEL_VALID_LOSS 102
#define ML_TRAIN_SUMMARY_MODEL_VALID_ACCURACY 103

class IniSection {
  friend class IniTestWrapper;

public:
  IniSection(const std::string &name) : section_name(name) {}

  IniSection(const std::string &section_name, const std::string &entry_str) :
    IniSection(section_name) {
    setEntry(entry_str);
  }

  /**
   * @brief copy entry from @a from and overwrite entry and section_name
   */
  IniSection(IniSection &from, const std::string &section_name,
             const std::string &entry_str) :
    IniSection(from) {
    if (!section_name.empty()) {
      this->section_name = section_name;
    }
    if (!entry_str.empty()) {
      setEntry(entry_str);
    }
  }

  IniSection(IniSection &from, const std::string &entry_str) :
    IniSection(from, "", entry_str) {}

  IniSection() = default;
  ~IniSection() = default;

  void print(std::ostream &out) {
    out << '[' << section_name << ']' << std::endl;
    for (auto &it : entry)
      out << it.first << " = " << it.second << std::endl;
  }

  IniSection &operator+=(const IniSection &rhs) {
    setEntry(rhs.entry);
    return *this;
  }

  IniSection operator+(const IniSection &rhs) const {
    return IniSection(*this) += rhs;
  }

  IniSection &operator+=(const std::string &s) {
    setEntry(s);
    return *this;
  }

  IniSection operator+(const std::string &s) { return IniSection(*this) += s; }

  void rename(const std::string &name) { section_name = name; }

  std::string getName() { return section_name; }

  bool operator==(const IniSection &rhs) const {
    return section_name == rhs.section_name && entry == rhs.entry;
  }

  bool operator!=(const IniSection &rhs) const { return !operator==(rhs); }

private:
  void setEntry(const std::unordered_map<std::string, std::string> &_entry) {
    for (auto &it : _entry) {
      this->entry[it.first] = it.second;
    }
  }

  /**
   * @brief setEntry as "Type = neuralnetwork | decayrate = 0.96 | -epochs = 1"
   * will delete epochs, and overwrite type and decayrate
   */
  void setEntry(const std::string &entry_str);

  std::string section_name;
  std::unordered_map<std::string, std::string> entry;

  friend std::ostream &operator<<(std::ostream &os, const IniSection &section) {
    return os << section.section_name;
  }
};

/**
 * @brief IniTestWrapper using IniSection
 *
 */
class IniTestWrapper {
public:
  using Sections = std::vector<IniSection>;

  /**
   * @brief Construct a new Ini Test Wrapper object
   *
   */
  IniTestWrapper(){};

  /**
   * @brief Construct a new Ini Test Wrapper object
   *
   * @param name_ name of the ini without `.ini` extension
   * @param sections_ sections that should go into ini
   */
  IniTestWrapper(const std::string &name_, const Sections &sections_ = {}) :
    name(name_),
    sections(sections_){};

  /**
   * @brief Get the Ini Name object
   *
   * @return std::string ini name with extension appended
   */
  std::string getIniName() { return name + ".ini"; }

  /**
   * @brief Get the Name
   *
   * @return std::string name
   */
  std::string getName() const { return name; }

  /**
   * @brief Get the Ini object
   *
   * @return std::ofstream ini file stream
   */
  std::ofstream getIni() {
    return getIni(getIniName().c_str(), std::ios_base::out);
  }

  /**
   * @brief Get the Ini object with given mode
   *
   * @return std::ofstream ini file stream
   */
  static std::ofstream getIni(const char *filename,
                              std::ios_base::openmode mode) {
    std::ofstream out(filename, mode);
    if (!out.good()) {
      throw std::runtime_error("cannot open ini");
    }
    return out;
  }

  /**
   * @brief save ini to a file
   */
  static void save_ini(const char *filename, std::vector<IniSection> sections,
                       std::ios_base::openmode mode = std::ios_base::out) {
    std::ofstream out = IniTestWrapper::getIni(filename, mode);

    for (auto &it : sections) {
      it.print(std::cout);
      std::cout << std::endl;
      it.print(out);
      out << std::endl;
    }

    out.close();
  }

  /**
   * @brief save ini to a file
   */
  void save_ini() { save_ini(getIniName().c_str(), sections); }

  /**
   * @brief erase ini
   *
   */
  void erase_ini() noexcept { remove(getIniName().c_str()); }

  bool operator==(const IniTestWrapper &rhs) const {
    return name == rhs.name && sections == rhs.sections;
  }

  bool operator!=(const IniTestWrapper &rhs) const { return !operator==(rhs); }

  /**
   * @brief update sections if section is empty, else update section by section
   * by key
   *
   * @param[in] s IniTestWrapper
   * @return IniTestWrapper& this
   */
  IniTestWrapper &operator+=(const IniTestWrapper &s) {
    if (sections.empty()) {
      sections = s.sections;
    } else {
      updateSections(s.sections);
    }

    return *this;
  }

  /**
   * @brief update sections if section is empty, else update section by section
   * by key
   *
   * @param[in] s IniTestWrapper
   * @return IniTestWrapper& a new instance
   */
  IniTestWrapper operator+(const IniTestWrapper &rhs) const {
    return IniTestWrapper(*this) += rhs;
  }

  IniTestWrapper &operator+=(const std::string &s) {
    updateSection(s);
    return *this;
  }

  IniTestWrapper operator+(const std::string &rhs) const {
    return IniTestWrapper(*this) += rhs;
  }

  friend std::ostream &operator<<(std::ostream &os, const IniTestWrapper &ini) {
    return os << ini.name;
  }

private:
  /**
   * @brief update a section from a formatted string, `sectionkey / propkey=val
   * | propkey=val`
   *
   * @param s "model/optimizer=SGD | ..."
   */
  void updateSection(const std::string &s);

  /**
   * @brief update Section that matches section key of @a s
   *
   * @param s section
   */
  void updateSection(const IniSection &s);

  /**
   * @brief update sections with following rule
   * if there is a section key, update entry of the section else throw
   * std::invalid_argument
   * @param sections sections to update
   */
  void updateSections(const Sections &sections_);

  std::string name;
  Sections sections;
};

/**
 * @brief This class wraps IniTestWrapper, this class must live longer than the
 * IniTestWrapper contained inside
 *
 */
class ScopedIni {
public:
  ScopedIni(const IniTestWrapper &ini_) : ini(ini_) { ini.save_ini(); }
  ScopedIni(const std::string &name_,
            const IniTestWrapper::Sections &sections_) :
    ini(name_, sections_) {
    ini.save_ini();
  }

  std::string getIniName() { return ini.getIniName(); }

  ~ScopedIni() { ini.erase_ini(); }

private:
  IniTestWrapper ini;
};

#define GEN_TEST_INPUT(input, eqation_i_j_k_l) \
  do {                                         \
    for (int i = 0; i < batch; ++i) {          \
      for (int j = 0; j < channel; ++j) {      \
        for (int k = 0; k < height; ++k) {     \
          for (int l = 0; l < width; ++l) {    \
            float val = eqation_i_j_k_l;       \
            input.setValue(i, j, k, l, val);   \
          }                                    \
        }                                      \
      }                                        \
    }                                          \
  } while (0)

/**
 * @brief return a tensor filled with contant value with dimension
 */
nntrainer::Tensor constant(float value, unsigned int batch, unsigned channel,
                           unsigned height, unsigned width);

/**
 * @brief return a tensor filled with ranged value with given dimension
 */
nntrainer::Tensor ranged(unsigned int batch, unsigned channel, unsigned height,
                         unsigned width);

/**
 * @brief return a tensor filled with random value with given dimension
 */
nntrainer::Tensor randUniform(unsigned int batch, unsigned channel,
                              unsigned height, unsigned width, float min = -1,
                              float max = 1);

/**
 * @brief replace string and save in file
 * @param[in] from string to be replaced
 * @param[in] to string to repalce with
 * @param[in] n file name to save
 * @retval void
 */
void replaceString(const std::string &from, const std::string &to,
                   const std::string n, std::string str);

/**
 * @brief      get data which size is batch for train
 * @param[out] outVec
 * @param[out] outLabel
 * @param[out] last if the data is finished
 * @param[in] user_data private data for the callback
 * @retval status for handling error
 */
int getBatch_train(float **outVec, float **outLabel, bool *last,
                   void *user_data);

/**
 * @brief      get data which size is batch for val
 * @param[out] outVec
 * @param[out] outLabel
 * @param[out] last if the data is finished
 * @param[in] user_data private data for the callback
 * @retval status for handling error
 */
int getBatch_val(float **outVec, float **outLabel, bool *last, void *user_data);

/**
 * @brief Get the Res Path object
 * @note if NNTRAINER_RESOURCE_PATH environment variable is given, @a
 * fallback_base is ignored and NNTRINAER_RESOURCE_PATH is directly used as a
 * base
 *
 * @param filename filename if omitted, ${prefix}/${base} will be returned
 * @param fallback_base list of base to attach when NNTRAINER_RESOURCE_PATH is
 * not given
 * @return const std::string path,
 */
const std::string
getResPath(const std::string &filename,
           const std::initializer_list<const char *> fallback_base = {});

#endif /* __cplusplus */
#endif /* __NNTRAINER_TEST_UTIL_H__ */
