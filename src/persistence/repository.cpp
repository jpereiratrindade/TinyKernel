#include "tinykernel/persistence/repository.hpp"
#include "tinykernel/evidence/digest.hpp"

#include <sqlite3.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string_view>

namespace tinykernel::persistence {
namespace {

using namespace ontology;

class Statement {
public:
  Statement(sqlite3 *database, const char *sql) : database_(database) {
    if (sqlite3_prepare_v2(database, sql, -1, &statement_, nullptr) != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(database));
    }
  }
  ~Statement() { sqlite3_finalize(statement_); }
  Statement(const Statement &) = delete;
  Statement &operator=(const Statement &) = delete;
  void bind(int index, const std::string &value) {
    if (sqlite3_bind_text(statement_, index, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) fail();
  }
  void bind(int index, std::uint32_t value) {
    if (sqlite3_bind_int64(statement_, index, static_cast<sqlite3_int64>(value)) != SQLITE_OK) fail();
  }
  void bind_null(int index) { if (sqlite3_bind_null(statement_, index) != SQLITE_OK) fail(); }
  [[nodiscard]] bool step_row() {
    const int status = sqlite3_step(statement_);
    if (status == SQLITE_ROW) return true;
    if (status == SQLITE_DONE) return false;
    fail();
  }
  void step_done() { if (sqlite3_step(statement_) != SQLITE_DONE) fail(); }
  [[nodiscard]] std::string text(int column) const {
    const auto *value = sqlite3_column_text(statement_, column);
    return value ? reinterpret_cast<const char *>(value) : std::string{};
  }
  [[nodiscard]] std::uint32_t integer(int column) const {
    return static_cast<std::uint32_t>(sqlite3_column_int64(statement_, column));
  }
  [[nodiscard]] bool is_null(int column) const { return sqlite3_column_type(statement_, column) == SQLITE_NULL; }
private:
  [[noreturn]] void fail() const { throw std::runtime_error(sqlite3_errmsg(database_)); }
  sqlite3 *database_{};
  sqlite3_stmt *statement_{};
};

void execute(sqlite3 *database, const char *sql) {
  char *message = nullptr;
  if (sqlite3_exec(database, sql, nullptr, nullptr, &message) != SQLITE_OK) {
    const std::string error = message ? message : sqlite3_errmsg(database);
    sqlite3_free(message);
    throw std::runtime_error(error);
  }
}

using Attributes = std::map<std::string, std::vector<std::string>, std::less<>>;

void save_entity(sqlite3 *database, const Identity &identity, const std::string &investigation,
                 const std::string &kind, const Attributes &attributes = {}) {
  Statement entity(database, R"SQL(
    INSERT INTO entities(id, investigation_id, kind, schema_version, ontology_version)
    VALUES(?,?,?,?,?)
    ON CONFLICT(id) DO UPDATE SET investigation_id=excluded.investigation_id,
      kind=excluded.kind, schema_version=excluded.schema_version,
      ontology_version=excluded.ontology_version)SQL");
  entity.bind(1, identity.id); entity.bind(2, investigation); entity.bind(3, kind);
  entity.bind(4, identity.schema_version); entity.bind(5, identity.ontology_version); entity.step_done();

  Statement clear(database, "DELETE FROM attributes WHERE entity_id=?");
  clear.bind(1, identity.id); clear.step_done();
  for (const auto &[key, values] : attributes) {
    for (std::size_t ordinal = 0; ordinal < values.size(); ++ordinal) {
      Statement attribute(database, "INSERT INTO attributes(entity_id,key,ordinal,value) VALUES(?,?,?,?)");
      attribute.bind(1, identity.id); attribute.bind(2, key);
      attribute.bind(3, static_cast<std::uint32_t>(ordinal)); attribute.bind(4, values[ordinal]);
      attribute.step_done();
    }
  }
}

void save_relation(sqlite3 *database, const std::string &investigation, const std::string &source,
                   const std::string &predicate, const std::string &target) {
  Statement statement(database, R"SQL(
    INSERT OR IGNORE INTO relations(investigation_id,source_id,predicate,target_id)
    VALUES(?,?,?,?))SQL");
  statement.bind(1, investigation); statement.bind(2, source); statement.bind(3, predicate);
  statement.bind(4, target); statement.step_done();
}

Attributes load_attributes(sqlite3 *database, const std::string &id) {
  Attributes result;
  Statement statement(database, "SELECT key,value FROM attributes WHERE entity_id=? ORDER BY key,ordinal");
  statement.bind(1, id);
  while (statement.step_row()) result[statement.text(0)].push_back(statement.text(1));
  return result;
}

std::string one(const Attributes &attributes, const std::string &key) {
  const auto found = attributes.find(key);
  return found == attributes.end() || found->second.empty() ? std::string{} : found->second.front();
}

std::vector<std::string> many(const Attributes &attributes, const std::string &key) {
  const auto found = attributes.find(key);
  return found == attributes.end() ? std::vector<std::string>{} : found->second;
}

std::vector<Identity> identities(sqlite3 *database, const std::string &investigation, const std::string &kind) {
  std::vector<Identity> result;
  Statement statement(database, R"SQL(
    SELECT id,schema_version,ontology_version FROM entities
    WHERE investigation_id=? AND kind=? ORDER BY id)SQL");
  statement.bind(1, investigation); statement.bind(2, kind);
  while (statement.step_row()) result.push_back({statement.text(0), statement.integer(1), statement.text(2)});
  return result;
}

Identity require_identity(sqlite3 *database, const std::string &id) {
  Statement statement(database, "SELECT id,schema_version,ontology_version FROM entities WHERE id=?");
  statement.bind(1, id);
  if (!statement.step_row()) throw std::runtime_error("entity not found: " + id);
  return {statement.text(0), statement.integer(1), statement.text(2)};
}

std::optional<std::string> optional_text(const Attributes &attributes, const std::string &key) {
  const auto value = one(attributes, key);
  return value.empty() ? std::nullopt : std::optional(value);
}

} // namespace

struct Repository::Impl { sqlite3 *database{}; };

Repository::Repository(std::filesystem::path path) : impl_(new Impl), path_(std::move(path)) {
  if (path_.has_parent_path()) std::filesystem::create_directories(path_.parent_path());
  if (sqlite3_open_v2(path_.string().c_str(), &impl_->database,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
    const std::string error = impl_->database ? sqlite3_errmsg(impl_->database) : "cannot open sqlite database";
    if (impl_->database) sqlite3_close(impl_->database);
    delete impl_; impl_ = nullptr;
    throw std::runtime_error(error);
  }
  execute(impl_->database, "PRAGMA foreign_keys=ON; PRAGMA journal_mode=WAL;");
}

Repository::~Repository() {
  if (impl_) { sqlite3_close(impl_->database); delete impl_; }
}

Repository::Repository(Repository &&other) noexcept : impl_(other.impl_), path_(std::move(other.path_)) {
  other.impl_ = nullptr;
}

Repository &Repository::operator=(Repository &&other) noexcept {
  if (this == &other) return *this;
  if (impl_) { sqlite3_close(impl_->database); delete impl_; }
  impl_ = other.impl_; path_ = std::move(other.path_); other.impl_ = nullptr;
  return *this;
}

void Repository::initialize() {
  execute(impl_->database, R"SQL(
    CREATE TABLE IF NOT EXISTS metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS entities(
      id TEXT PRIMARY KEY, investigation_id TEXT NOT NULL, kind TEXT NOT NULL,
      schema_version INTEGER NOT NULL, ontology_version TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS investigations(
      id TEXT PRIMARY KEY REFERENCES entities(id), title TEXT NOT NULL, phenomenon_id TEXT NOT NULL,
      context_id TEXT NOT NULL, profile_id TEXT NOT NULL, reduction_order TEXT NOT NULL, status TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS attributes(
      entity_id TEXT NOT NULL REFERENCES entities(id), key TEXT NOT NULL, ordinal INTEGER NOT NULL,
      value TEXT NOT NULL, PRIMARY KEY(entity_id,key,ordinal));
    CREATE TABLE IF NOT EXISTS relations(
      investigation_id TEXT NOT NULL, source_id TEXT NOT NULL, predicate TEXT NOT NULL, target_id TEXT NOT NULL,
      UNIQUE(investigation_id,source_id,predicate,target_id));
    CREATE TABLE IF NOT EXISTS runs(
      id TEXT PRIMARY KEY REFERENCES entities(id), investigation_id TEXT NOT NULL,
      intervention_id TEXT, source_realization_id TEXT NOT NULL, result_realization_id TEXT NOT NULL,
      status TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS evidence_records(
      id TEXT PRIMARY KEY REFERENCES entities(id), investigation_id TEXT NOT NULL, run_id TEXT NOT NULL,
      witness_id TEXT NOT NULL, artifact TEXT NOT NULL, sha256 TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS evidence_observations(
      evidence_id TEXT NOT NULL REFERENCES evidence_records(id), observation_id TEXT NOT NULL,
      ordinal INTEGER NOT NULL, PRIMARY KEY(evidence_id,ordinal));
    CREATE TABLE IF NOT EXISTS claims(
      id TEXT PRIMARY KEY REFERENCES entities(id), investigation_id TEXT NOT NULL, subject TEXT NOT NULL,
      assertion TEXT NOT NULL, phenomenon_id TEXT NOT NULL, context_id TEXT NOT NULL,
      level TEXT NOT NULL, status TEXT NOT NULL, limitations TEXT NOT NULL, provenance_id TEXT NOT NULL);
    CREATE TABLE IF NOT EXISTS claim_scopes(
      claim_id TEXT NOT NULL REFERENCES claims(id), scope_kind TEXT NOT NULL, ordinal INTEGER NOT NULL,
      value TEXT NOT NULL, PRIMARY KEY(claim_id,scope_kind,ordinal));
    CREATE TRIGGER IF NOT EXISTS evidence_no_update BEFORE UPDATE ON evidence_records
      BEGIN SELECT RAISE(ABORT, 'evidence is immutable'); END;
    CREATE TRIGGER IF NOT EXISTS evidence_no_delete BEFORE DELETE ON evidence_records
      BEGIN SELECT RAISE(ABORT, 'evidence is immutable'); END;
    CREATE TRIGGER IF NOT EXISTS evidence_observations_no_update BEFORE UPDATE ON evidence_observations
      BEGIN SELECT RAISE(ABORT, 'evidence is immutable'); END;
    CREATE TRIGGER IF NOT EXISTS evidence_observations_no_delete BEFORE DELETE ON evidence_observations
      BEGIN SELECT RAISE(ABORT, 'evidence is immutable'); END;
    INSERT OR REPLACE INTO metadata(key,value) VALUES('ontology_id','TK-O');
    INSERT OR REPLACE INTO metadata(key,value) VALUES('ontology_version','0.2.0');
    INSERT OR REPLACE INTO metadata(key,value) VALUES('schema_version','1');
  )SQL");
}

void Repository::save(const Study &study) {
  const auto inv = study.investigation.identity.id;
  execute(impl_->database, "BEGIN IMMEDIATE");
  try {
    save_entity(impl_->database, study.investigation.identity, inv, "Investigation");
    {
      Statement statement(impl_->database, R"SQL(
        INSERT INTO investigations(id,title,phenomenon_id,context_id,profile_id,reduction_order,status)
        VALUES(?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET title=excluded.title,
        phenomenon_id=excluded.phenomenon_id,context_id=excluded.context_id,profile_id=excluded.profile_id,
        reduction_order=excluded.reduction_order,status=excluded.status)SQL");
      statement.bind(1, inv); statement.bind(2, study.investigation.title);
      statement.bind(3, study.investigation.phenomenon_id); statement.bind(4, study.investigation.context_id);
      statement.bind(5, study.investigation.constitutive_profile_id);
      statement.bind(6, study.investigation.reduction_order); statement.bind(7, study.investigation.status);
      statement.step_done();
    }
    save_entity(impl_->database, study.phenomenon.identity, inv, "Phenomenon",
                {{"name", {study.phenomenon.name}}, {"definition", {study.phenomenon.definition}}});
    save_entity(impl_->database, study.context.identity, inv, "Context", {{"description", {study.context.description}}});
    save_entity(impl_->database, study.constitutive_profile.identity, inv, "ConstitutiveProfile",
                {{"distinction", study.constitutive_profile.distinctions},
                 {"relation", study.constitutive_profile.relations},
                 {"temporal_constraint", study.constitutive_profile.temporal_constraints}});

    for (const auto &item : study.realizations) save_entity(impl_->database, item.identity, inv, "Realization",
        {{"label", {item.label}}, {"component", item.components}, {"reduction_rank", {std::to_string(item.reduction_rank)}}});
    for (const auto &item : study.structures) save_entity(impl_->database, item.identity, inv, "Structure",
        {{"realization_id", {item.realization_id}}, {"part", item.parts}, {"relation", item.relations}});
    for (const auto &item : study.interventions) {
      Attributes attrs{{"kind", {item.kind}}, {"source_realization_id", {item.source_realization_id}},
                       {"target", {item.target}}, {"replacement", {item.replacement}},
                       {"prediction", {item.prediction}}, {"status", {item.status}}};
      if (item.target_realization_id) attrs["target_realization_id"] = {*item.target_realization_id};
      save_entity(impl_->database, item.identity, inv, "Intervention", attrs);
    }
    for (const auto &item : study.observations) save_entity(impl_->database, item.identity, inv, "Observation",
        {{"run_id", {item.run_id}}, {"realization_id", {item.realization_id}}, {"witness_id", {item.witness_id}},
         {"dimension", {item.dimension}}, {"value", {item.value}}, {"satisfied", {item.satisfied ? "1" : "0"}}});
    for (const auto &item : study.witnesses) save_entity(impl_->database, item.identity, inv, "Witness",
        {{"kind", {item.kind}}, {"description", {item.description}}});
    for (const auto &item : study.adjudications) save_entity(impl_->database, item.identity, inv, "Adjudication",
        {{"run_id", {item.run_id}}, {"outcome", {to_string(item.outcome)}}, {"classification", {item.classification}},
         {"rule", {item.rule}}, {"rationale", {item.rationale}}, {"evidence_reference", item.evidence_references}});
    for (const auto &item : study.provenance) save_entity(impl_->database, item.identity, inv, "Provenance",
        {{"source", {item.source}}, {"method", {item.method}}, {"timestamp", {item.timestamp}}, {"detail", {item.detail}}});

    for (const auto &item : study.runs) {
      save_entity(impl_->database, item.identity, inv, "Run");
      Statement statement(impl_->database, R"SQL(
        INSERT INTO runs(id,investigation_id,intervention_id,source_realization_id,result_realization_id,status)
        VALUES(?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET status=excluded.status)SQL");
      statement.bind(1, item.identity.id); statement.bind(2, inv);
      if (item.intervention_id) statement.bind(3, *item.intervention_id); else statement.bind_null(3);
      statement.bind(4, item.source_realization_id); statement.bind(5, item.result_realization_id);
      statement.bind(6, item.status); statement.step_done();
    }

    for (const auto &item : study.evidence) {
      Statement existing(impl_->database, "SELECT artifact,sha256 FROM evidence_records WHERE id=?");
      existing.bind(1, item.identity.id);
      if (existing.step_row()) {
        if (existing.text(0) != item.artifact || existing.text(1) != item.sha256) {
          throw std::runtime_error("evidence is immutable: " + item.identity.id);
        }
        continue;
      }
      save_entity(impl_->database, item.identity, inv, "Evidence");
      Statement statement(impl_->database, R"SQL(
        INSERT INTO evidence_records(id,investigation_id,run_id,witness_id,artifact,sha256)
        VALUES(?,?,?,?,?,?))SQL");
      statement.bind(1, item.identity.id); statement.bind(2, inv); statement.bind(3, item.run_id);
      statement.bind(4, item.witness_id); statement.bind(5, item.artifact); statement.bind(6, item.sha256);
      statement.step_done();
      for (std::size_t ordinal = 0; ordinal < item.observation_ids.size(); ++ordinal) {
        Statement observation(impl_->database,
            "INSERT INTO evidence_observations(evidence_id,observation_id,ordinal) VALUES(?,?,?)");
        observation.bind(1, item.identity.id); observation.bind(2, item.observation_ids[ordinal]);
        observation.bind(3, static_cast<std::uint32_t>(ordinal)); observation.step_done();
      }
    }

    for (const auto &item : study.claims) {
      save_entity(impl_->database, item.identity, inv, "Claim");
      Statement statement(impl_->database, R"SQL(
        INSERT INTO claims(id,investigation_id,subject,assertion,phenomenon_id,context_id,level,status,limitations,provenance_id)
        VALUES(?,?,?,?,?,?,?,?,?,?) ON CONFLICT(id) DO UPDATE SET subject=excluded.subject,
        assertion=excluded.assertion,level=excluded.level,status=excluded.status,
        limitations=excluded.limitations,provenance_id=excluded.provenance_id)SQL");
      statement.bind(1, item.identity.id); statement.bind(2, inv); statement.bind(3, item.subject);
      statement.bind(4, item.assertion); statement.bind(5, item.phenomenon_id); statement.bind(6, item.context_id);
      statement.bind(7, to_string(item.level)); statement.bind(8, to_string(item.status));
      statement.bind(9, item.limitations); statement.bind(10, item.provenance_id); statement.step_done();
      Statement clear(impl_->database, "DELETE FROM claim_scopes WHERE claim_id=?");
      clear.bind(1, item.identity.id); clear.step_done();
      const std::array scopes{
        std::pair{"intervention", &item.intervention_scope}, std::pair{"witness", &item.witness_scope},
        std::pair{"evidence", &item.evidence_references}};
      for (const auto &[kind, values] : scopes) for (std::size_t ordinal = 0; ordinal < values->size(); ++ordinal) {
        Statement scope(impl_->database, "INSERT INTO claim_scopes(claim_id,scope_kind,ordinal,value) VALUES(?,?,?,?)");
        scope.bind(1, item.identity.id); scope.bind(2, kind);
        scope.bind(3, static_cast<std::uint32_t>(ordinal)); scope.bind(4, (*values)[ordinal]); scope.step_done();
      }
    }

    { Statement clear(impl_->database, "DELETE FROM relations WHERE investigation_id=?"); clear.bind(1, inv); clear.step_done(); }
    save_relation(impl_->database, inv, study.phenomenon.identity.id, "specified_by", study.constitutive_profile.identity.id);
    save_relation(impl_->database, inv, study.phenomenon.identity.id, "situated_in", study.context.identity.id);
    for (const auto &item : study.realizations) save_relation(impl_->database, inv, item.identity.id, "candidate", study.phenomenon.identity.id);
    for (const auto &item : study.structures) save_relation(impl_->database, inv, item.realization_id, "has_structure", item.identity.id);
    for (const auto &item : study.interventions) {
      save_relation(impl_->database, inv, item.identity.id, "transforms_from", item.source_realization_id);
      if (item.target_realization_id) save_relation(impl_->database, inv, item.identity.id, "transforms_to", *item.target_realization_id);
    }
    for (const auto &item : study.observations) save_relation(impl_->database, inv, item.identity.id, "observed_by", item.witness_id);
    for (const auto &item : study.evidence) save_relation(impl_->database, inv, item.identity.id, "produced_by", item.witness_id);
    for (const auto &item : study.adjudications) for (const auto &reference : item.evidence_references)
      save_relation(impl_->database, inv, item.identity.id, "adjudicates", reference);
    for (const auto &item : study.claims) {
      save_relation(impl_->database, inv, item.identity.id, "has_provenance", item.provenance_id);
      for (const auto &reference : item.evidence_references) save_relation(impl_->database, inv, reference, "supports", item.identity.id);
    }
    execute(impl_->database, "COMMIT");
  } catch (...) {
    execute(impl_->database, "ROLLBACK");
    throw;
  }
}

Study Repository::load(const std::string &inv) const {
  Study study;
  {
    Statement statement(impl_->database, R"SQL(
      SELECT title,phenomenon_id,context_id,profile_id,reduction_order,status
      FROM investigations WHERE id=?)SQL");
    statement.bind(1, inv);
    if (!statement.step_row()) throw std::runtime_error("investigation not found: " + inv);
    study.investigation = {require_identity(impl_->database, inv), statement.text(0), statement.text(1),
                           statement.text(2), statement.text(3), statement.text(4), statement.text(5)};
  }
  {
    auto identity = require_identity(impl_->database, study.investigation.phenomenon_id);
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.phenomenon = {identity, one(attrs, "name"), one(attrs, "definition")};
  }
  {
    auto identity = require_identity(impl_->database, study.investigation.context_id);
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.context = {identity, one(attrs, "description")};
  }
  {
    auto identity = require_identity(impl_->database, study.investigation.constitutive_profile_id);
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.constitutive_profile = {identity, many(attrs, "distinction"), many(attrs, "relation"), many(attrs, "temporal_constraint")};
  }
  for (auto identity : identities(impl_->database, inv, "Realization")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.realizations.push_back({identity, inv, one(attrs, "label"), many(attrs, "component"),
                                  static_cast<std::uint32_t>(std::stoul(one(attrs, "reduction_rank")))});
  }
  for (auto identity : identities(impl_->database, inv, "Structure")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.structures.push_back({identity, one(attrs, "realization_id"), many(attrs, "part"), many(attrs, "relation")});
  }
  for (auto identity : identities(impl_->database, inv, "Intervention")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.interventions.push_back({identity, inv, one(attrs, "kind"), one(attrs, "source_realization_id"),
        optional_text(attrs, "target_realization_id"), one(attrs, "target"), one(attrs, "replacement"),
        one(attrs, "prediction"), one(attrs, "status")});
  }
  for (auto identity : identities(impl_->database, inv, "Observation")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.observations.push_back({identity, one(attrs, "run_id"), one(attrs, "realization_id"),
        one(attrs, "witness_id"), one(attrs, "dimension"), one(attrs, "value"), one(attrs, "satisfied") == "1"});
  }
  for (auto identity : identities(impl_->database, inv, "Witness")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.witnesses.push_back({identity, one(attrs, "kind"), one(attrs, "description")});
  }
  for (auto identity : identities(impl_->database, inv, "Adjudication")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.adjudications.push_back({identity, one(attrs, "run_id"), outcome_from_string(one(attrs, "outcome")),
        one(attrs, "classification"), one(attrs, "rule"), one(attrs, "rationale"), many(attrs, "evidence_reference")});
  }
  for (auto identity : identities(impl_->database, inv, "Provenance")) {
    const auto attrs = load_attributes(impl_->database, identity.id);
    study.provenance.push_back({identity, one(attrs, "source"), one(attrs, "method"), one(attrs, "timestamp"), one(attrs, "detail")});
  }
  {
    Statement statement(impl_->database, R"SQL(
      SELECT id,intervention_id,source_realization_id,result_realization_id,status
      FROM runs WHERE investigation_id=? ORDER BY id)SQL");
    statement.bind(1, inv);
    while (statement.step_row()) study.runs.push_back({require_identity(impl_->database, statement.text(0)), inv,
        statement.is_null(1) ? std::nullopt : std::optional(statement.text(1)), statement.text(2), statement.text(3), statement.text(4)});
  }
  {
    Statement statement(impl_->database, R"SQL(
      SELECT id,run_id,witness_id,artifact,sha256 FROM evidence_records
      WHERE investigation_id=? ORDER BY id)SQL");
    statement.bind(1, inv);
    while (statement.step_row()) {
      Evidence item{require_identity(impl_->database, statement.text(0)), statement.text(1), statement.text(2), {}, statement.text(3), statement.text(4)};
      Statement observations(impl_->database,
          "SELECT observation_id FROM evidence_observations WHERE evidence_id=? ORDER BY ordinal");
      observations.bind(1, item.identity.id);
      while (observations.step_row()) item.observation_ids.push_back(observations.text(0));
      study.evidence.push_back(std::move(item));
    }
  }
  {
    Statement statement(impl_->database, R"SQL(
      SELECT id,subject,assertion,phenomenon_id,context_id,level,status,limitations,provenance_id
      FROM claims WHERE investigation_id=? ORDER BY id)SQL");
    statement.bind(1, inv);
    while (statement.step_row()) {
      Claim item{require_identity(impl_->database, statement.text(0)), statement.text(1), statement.text(2),
          statement.text(3), statement.text(4), {}, {}, {}, claim_level_from_string(statement.text(5)),
          claim_status_from_string(statement.text(6)), statement.text(7), statement.text(8)};
      Statement scopes(impl_->database,
          "SELECT scope_kind,value FROM claim_scopes WHERE claim_id=? ORDER BY scope_kind,ordinal");
      scopes.bind(1, item.identity.id);
      while (scopes.step_row()) {
        if (scopes.text(0) == "intervention") item.intervention_scope.push_back(scopes.text(1));
        if (scopes.text(0) == "witness") item.witness_scope.push_back(scopes.text(1));
        if (scopes.text(0) == "evidence") item.evidence_references.push_back(scopes.text(1));
      }
      study.claims.push_back(std::move(item));
    }
  }
  return study;
}

std::vector<std::string> Repository::list() const {
  std::vector<std::string> result;
  Statement statement(impl_->database, "SELECT id FROM investigations ORDER BY id");
  while (statement.step_row()) result.push_back(statement.text(0));
  return result;
}

bool Repository::verify_integrity(std::string &detail) const {
  Statement integrity(impl_->database, "PRAGMA integrity_check");
  if (!integrity.step_row() || integrity.text(0) != "ok") { detail = "SQLite integrity_check failed"; return false; }
  Statement ontology(impl_->database, "SELECT value FROM metadata WHERE key='ontology_version'");
  if (!ontology.step_row() || ontology.text(0) != "0.2.0") { detail = "ontology version mismatch"; return false; }
  Statement evidence(impl_->database, "SELECT id,artifact,sha256 FROM evidence_records ORDER BY id");
  while (evidence.step_row()) {
    if (tinykernel::evidence::sha256(evidence.text(1)) != evidence.text(2)) {
      detail = "evidence digest mismatch: " + evidence.text(0); return false;
    }
  }
  detail = "SQLite integrity, TK-O version, and evidence digests are valid";
  return true;
}

const std::filesystem::path &Repository::path() const noexcept { return path_; }

} // namespace tinykernel::persistence
