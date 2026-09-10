#pragma once

#include "tinykernel/ontology/types.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace tinykernel::persistence {

class Repository {
public:
  explicit Repository(std::filesystem::path database_path);
  ~Repository();
  Repository(const Repository &) = delete;
  Repository &operator=(const Repository &) = delete;
  Repository(Repository &&) noexcept;
  Repository &operator=(Repository &&) noexcept;

  void initialize();
  void save(const ontology::Study &study);
  [[nodiscard]] ontology::Study load(const std::string &investigation_id) const;
  [[nodiscard]] std::vector<std::string> list() const;
  [[nodiscard]] bool verify_integrity(std::string &detail) const;
  [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
  struct Impl;
  Impl *impl_{};
  std::filesystem::path path_;
};

[[nodiscard]] std::string deterministic_export(const ontology::Study &study);

} // namespace tinykernel::persistence
