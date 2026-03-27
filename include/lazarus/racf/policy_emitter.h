// Lazarus Systems -- IAM/RBAC Policy Emitter
// Tier 8: RACF Profile Importer -- JSON, Terraform HCL, Kubernetes RBAC output

#ifndef LAZARUS_RACF_POLICY_EMITTER_H
#define LAZARUS_RACF_POLICY_EMITTER_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <functional>
#include <cctype>

#include "lazarus/racf/profile.h"

namespace lazarus { namespace racf {

// JSON string escaping
inline std::string json_escape(std::string_view sv) {
    std::string out;
    out.reserve(sv.size() + 8);
    for (char c : sv) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Lowercase/sanitize identifier for Terraform/K8s names
inline std::string sanitize_id(std::string_view sv) {
    std::string out;
    out.reserve(sv.size());
    for (char c : sv) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else {
            out += '_';
        }
    }
    // strip leading/trailing underscores
    while (!out.empty() && out.front() == '_') out.erase(out.begin());
    while (!out.empty() && out.back() == '_') out.pop_back();
    if (out.empty()) out = "unnamed";
    return out;
}

// Map AccessLevel to IAM actions
inline std::vector<std::string> access_to_iam_actions(AccessLevel level) {
    switch (level) {
        case AccessLevel::NONE:    return {};
        case AccessLevel::READ:    return {"s3:GetObject", "s3:ListBucket"};
        case AccessLevel::UPDATE:  return {"s3:GetObject", "s3:ListBucket", "s3:PutObject"};
        case AccessLevel::CONTROL: return {"s3:GetObject", "s3:ListBucket", "s3:PutObject", "s3:DeleteObject"};
        case AccessLevel::ALTER:   return {"s3:*"};
    }
    return {};
}

// Map AccessLevel to K8s verbs
inline std::vector<std::string> access_to_k8s_verbs(AccessLevel level) {
    switch (level) {
        case AccessLevel::NONE:    return {};
        case AccessLevel::READ:    return {"get", "list"};
        case AccessLevel::UPDATE:  return {"get", "list", "create", "update"};
        case AccessLevel::CONTROL: return {"get", "list", "create", "update", "delete"};
        case AccessLevel::ALTER:   return {"get", "list", "create", "update", "delete", "patch", "watch"};
    }
    return {};
}

// Map AccessLevel to Terraform policy effect string
inline std::string access_to_effect(AccessLevel level) {
    return level == AccessLevel::NONE ? "Deny" : "Allow";
}

// ============================================================
// JSON IAM Policy Emitter
// ============================================================

class JsonPolicyEmitter {
public:
    // Emit full IAM policy document for a ProfileDatabase
    static std::string emit(const ProfileDatabase& db) {
        std::ostringstream os;
        os << "{\n";
        os << "  \"Version\": \"2012-10-17\",\n";
        os << "  \"Users\": [\n";
        emit_users(os, db);
        os << "  ],\n";
        os << "  \"Groups\": [\n";
        emit_groups(os, db);
        os << "  ],\n";
        os << "  \"Policies\": [\n";
        emit_dataset_policies(os, db);
        os << "  ]\n";
        os << "}";
        return os.str();
    }

    // Emit a single user IAM policy
    static std::string emit_user(const UserProfile& u, const ProfileDatabase& db) {
        std::ostringstream os;
        os << "{\n";
        os << "  \"UserName\": \"" << json_escape(u.userid) << "\",\n";
        os << "  \"DisplayName\": \"" << json_escape(u.name) << "\",\n";

        // Group memberships
        auto conns = db.user_connects(u.userid);
        os << "  \"Groups\": [";
        for (size_t i = 0; i < conns.size(); ++i) {
            if (i > 0) os << ", ";
            os << "\"" << json_escape(conns[i].group) << "\"";
        }
        os << "],\n";

        // Admin flags
        os << "  \"IsAdmin\": " << (u.special ? "true" : "false") << ",\n";
        os << "  \"IsOperator\": " << (u.operations ? "true" : "false") << ",\n";
        os << "  \"IsAuditor\": " << (u.auditor ? "true" : "false") << ",\n";
        os << "  \"IsRevoked\": " << (u.revoke ? "true" : "false") << "\n";
        os << "}";
        return os.str();
    }

    // Emit a dataset resource policy
    static std::string emit_dataset_policy(const DatasetProfile& ds) {
        std::ostringstream os;
        os << "{\n";
        os << "  \"Resource\": \"" << json_escape(ds.dsname) << "\",\n";
        os << "  \"IsGeneric\": " << (ds.generic ? "true" : "false") << ",\n";
        os << "  \"DefaultAccess\": \"" << access_level_name(ds.uacc) << "\",\n";
        os << "  \"Statements\": [\n";

        for (size_t i = 0; i < ds.access_list.size(); ++i) {
            auto& ae = ds.access_list[i];
            os << "    {\n";
            os << "      \"Effect\": \"" << access_to_effect(ae.access) << "\",\n";
            os << "      \"Principal\": \"" << json_escape(ae.entity) << "\",\n";
            os << "      \"Actions\": [";
            auto actions = access_to_iam_actions(ae.access);
            for (size_t j = 0; j < actions.size(); ++j) {
                if (j > 0) os << ", ";
                os << "\"" << actions[j] << "\"";
            }
            os << "]\n";
            os << "    }";
            if (i + 1 < ds.access_list.size()) os << ",";
            os << "\n";
        }
        os << "  ]\n";
        os << "}";
        return os.str();
    }

private:
    static void emit_users(std::ostringstream& os, const ProfileDatabase& db) {
        for (size_t i = 0; i < db.users.size(); ++i) {
            auto& u = db.users[i];
            auto conns = db.user_connects(u.userid);
            os << "    {\n";
            os << "      \"UserName\": \"" << json_escape(u.userid) << "\",\n";
            os << "      \"DisplayName\": \"" << json_escape(u.name) << "\",\n";
            os << "      \"DefaultGroup\": \"" << json_escape(u.dfltgrp) << "\",\n";
            os << "      \"Groups\": [";
            for (size_t j = 0; j < conns.size(); ++j) {
                if (j > 0) os << ", ";
                os << "\"" << json_escape(conns[j].group) << "\"";
            }
            os << "],\n";
            os << "      \"IsAdmin\": " << (u.special ? "true" : "false") << ",\n";
            os << "      \"IsRevoked\": " << (u.revoke ? "true" : "false") << "\n";
            os << "    }";
            if (i + 1 < db.users.size()) os << ",";
            os << "\n";
        }
    }

    static void emit_groups(std::ostringstream& os, const ProfileDatabase& db) {
        for (size_t i = 0; i < db.groups.size(); ++i) {
            auto& g = db.groups[i];
            auto members = db.group_connects(g.group);
            os << "    {\n";
            os << "      \"GroupName\": \"" << json_escape(g.group) << "\",\n";
            os << "      \"SuperGroup\": \"" << json_escape(g.supgroup) << "\",\n";
            os << "      \"Members\": [";
            for (size_t j = 0; j < members.size(); ++j) {
                if (j > 0) os << ", ";
                os << "\"" << json_escape(members[j].userid) << "\"";
            }
            os << "]\n";
            os << "    }";
            if (i + 1 < db.groups.size()) os << ",";
            os << "\n";
        }
    }

    static void emit_dataset_policies(std::ostringstream& os, const ProfileDatabase& db) {
        for (size_t i = 0; i < db.datasets.size(); ++i) {
            auto& ds = db.datasets[i];
            os << "    {\n";
            os << "      \"Resource\": \"" << json_escape(ds.dsname) << "\",\n";
            os << "      \"DefaultAccess\": \"" << access_level_name(ds.uacc) << "\",\n";
            os << "      \"Statements\": [";
            for (size_t j = 0; j < ds.access_list.size(); ++j) {
                auto& ae = ds.access_list[j];
                if (j > 0) os << ", ";
                os << "{\n";
                os << "        \"Principal\": \"" << json_escape(ae.entity) << "\",\n";
                os << "        \"Effect\": \"" << access_to_effect(ae.access) << "\",\n";
                os << "        \"Access\": \"" << access_level_name(ae.access) << "\"\n";
                os << "      }";
            }
            os << "]\n";
            os << "    }";
            if (i + 1 < db.datasets.size()) os << ",";
            os << "\n";
        }
    }
};

// ============================================================
// Terraform HCL Emitter
// ============================================================

class TerraformEmitter {
public:
    // Emit Terraform HCL for all profiles
    static std::string emit(const ProfileDatabase& db) {
        std::ostringstream os;
        emit_header(os);
        emit_users_tf(os, db);
        emit_groups_tf(os, db);
        emit_memberships_tf(os, db);
        emit_policies_tf(os, db);
        emit_attachments_tf(os, db);
        return os.str();
    }

    // Emit a single user resource
    static std::string emit_user(const UserProfile& u) {
        std::ostringstream os;
        auto id = sanitize_id(u.userid);
        os << "resource \"aws_iam_user\" \"" << id << "\" {\n";
        os << "  name = \"" << u.userid << "\"\n";
        os << "  tags = {\n";
        os << "    racf_owner    = \"" << u.owner << "\"\n";
        os << "    racf_dfltgrp  = \"" << u.dfltgrp << "\"\n";
        os << "    racf_special  = \"" << (u.special ? "true" : "false") << "\"\n";
        os << "    racf_revoked  = \"" << (u.revoke ? "true" : "false") << "\"\n";
        os << "  }\n";
        os << "}\n";
        return os.str();
    }

    // Emit a single group resource
    static std::string emit_group(const GroupProfile& g) {
        std::ostringstream os;
        auto id = sanitize_id(g.group);
        os << "resource \"aws_iam_group\" \"" << id << "\" {\n";
        os << "  name = \"" << g.group << "\"\n";
        os << "}\n";
        return os.str();
    }

    // Emit a dataset policy
    static std::string emit_dataset_policy(const DatasetProfile& ds) {
        std::ostringstream os;
        auto id = sanitize_id(ds.dsname);
        os << "resource \"aws_iam_policy\" \"ds_" << id << "\" {\n";
        os << "  name        = \"racf-ds-" << id << "\"\n";
        os << "  description = \"RACF dataset policy for " << ds.dsname << "\"\n";
        os << "  policy = jsonencode({\n";
        os << "    Version = \"2012-10-17\"\n";
        os << "    Statement = [\n";

        if (!ds.access_list.empty()) {
            for (size_t i = 0; i < ds.access_list.size(); ++i) {
                auto& ae = ds.access_list[i];
                auto actions = access_to_iam_actions(ae.access);
                os << "      {\n";
                os << "        Effect   = \"" << access_to_effect(ae.access) << "\"\n";
                os << "        Action   = [";
                for (size_t j = 0; j < actions.size(); ++j) {
                    if (j > 0) os << ", ";
                    os << "\"" << actions[j] << "\"";
                }
                os << "]\n";
                os << "        Resource = \"arn:aws:s3:::" << ds.dsname << "/*\"\n";
                os << "      }";
                if (i + 1 < ds.access_list.size()) os << ",";
                os << "\n";
            }
        } else {
            // Default UACC policy
            auto actions = access_to_iam_actions(ds.uacc);
            if (!actions.empty()) {
                os << "      {\n";
                os << "        Effect   = \"" << access_to_effect(ds.uacc) << "\"\n";
                os << "        Action   = [";
                for (size_t j = 0; j < actions.size(); ++j) {
                    if (j > 0) os << ", ";
                    os << "\"" << actions[j] << "\"";
                }
                os << "]\n";
                os << "        Resource = \"arn:aws:s3:::" << ds.dsname << "/*\"\n";
                os << "      }\n";
            }
        }
        os << "    ]\n";
        os << "  })\n";
        os << "}\n";
        return os.str();
    }

private:
    static void emit_header(std::ostringstream& os) {
        os << "# RACF-to-Terraform IAM policy -- generated by Lazarus Systems\n\n";
    }

    static void emit_users_tf(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& u : db.users) {
            os << emit_user(u) << "\n";
        }
    }

    static void emit_groups_tf(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& g : db.groups) {
            os << emit_group(g) << "\n";
        }
    }

    static void emit_memberships_tf(std::ostringstream& os, const ProfileDatabase& db) {
        // Group connect records into memberships
        std::map<std::string, std::vector<std::string>> memberships;
        for (auto& c : db.connects) {
            memberships[c.group].push_back(c.userid);
        }
        for (auto& [grp, users] : memberships) {
            auto id = sanitize_id(grp);
            os << "resource \"aws_iam_group_membership\" \"" << id << "_members\" {\n";
            os << "  name  = \"" << grp << "-membership\"\n";
            os << "  group = aws_iam_group." << id << ".name\n";
            os << "  users = [\n";
            for (auto& u : users) {
                os << "    aws_iam_user." << sanitize_id(u) << ".name,\n";
            }
            os << "  ]\n";
            os << "}\n\n";
        }
    }

    static void emit_policies_tf(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& ds : db.datasets) {
            os << emit_dataset_policy(ds) << "\n";
        }
    }

    static void emit_attachments_tf(std::ostringstream& os, const ProfileDatabase& db) {
        // Attach dataset policies to users who have direct access
        for (auto& ds : db.datasets) {
            auto dsid = sanitize_id(ds.dsname);
            for (auto& ae : ds.access_list) {
                // Check if entity is a user
                if (db.find_user(ae.entity).has_value()) {
                    auto uid = sanitize_id(ae.entity);
                    os << "resource \"aws_iam_user_policy_attachment\" \""
                       << uid << "_" << dsid << "\" {\n";
                    os << "  user       = aws_iam_user." << uid << ".name\n";
                    os << "  policy_arn = aws_iam_policy.ds_" << dsid << ".arn\n";
                    os << "}\n\n";
                }
            }
        }
    }
};

// ============================================================
// Kubernetes RBAC Emitter
// ============================================================

class KubernetesRbacEmitter {
public:
    // Emit full K8s RBAC YAML
    static std::string emit(const ProfileDatabase& db) {
        std::ostringstream os;
        emit_service_accounts(os, db);
        emit_roles(os, db);
        emit_role_bindings(os, db);
        emit_admin_roles(os, db);
        return os.str();
    }

    // Emit a single ServiceAccount
    static std::string emit_service_account(const UserProfile& u) {
        std::ostringstream os;
        auto id = sanitize_id(u.userid);
        os << "apiVersion: v1\n";
        os << "kind: ServiceAccount\n";
        os << "metadata:\n";
        os << "  name: " << id << "\n";
        os << "  labels:\n";
        os << "    racf-userid: \"" << u.userid << "\"\n";
        os << "    racf-owner: \"" << u.owner << "\"\n";
        if (u.special) {
            os << "  annotations:\n";
            os << "    racf-special: \"true\"\n";
        }
        return os.str();
    }

    // Emit a Role from a group
    static std::string emit_role(const GroupProfile& g,
                                  const ProfileDatabase& db) {
        std::ostringstream os;
        auto id = sanitize_id(g.group);

        // Determine the highest access level for this group across all datasets
        AccessLevel max_level = AccessLevel::NONE;
        for (auto& ds : db.datasets) {
            for (auto& ae : ds.access_list) {
                if (ae.entity == g.group && ae.access > max_level) {
                    max_level = ae.access;
                }
            }
        }

        auto verbs = access_to_k8s_verbs(max_level);
        os << "apiVersion: rbac.authorization.k8s.io/v1\n";
        os << "kind: Role\n";
        os << "metadata:\n";
        os << "  name: racf-" << id << "\n";
        os << "  labels:\n";
        os << "    racf-group: \"" << g.group << "\"\n";
        os << "rules:\n";
        os << "  - apiGroups: [\"\"]\n";
        os << "    resources: [\"pods\", \"services\", \"configmaps\"]\n";
        os << "    verbs: [";
        for (size_t i = 0; i < verbs.size(); ++i) {
            if (i > 0) os << ", ";
            os << "\"" << verbs[i] << "\"";
        }
        os << "]\n";
        return os.str();
    }

    // Emit a RoleBinding connecting user to group role
    static std::string emit_role_binding(const ConnectProfile& c) {
        std::ostringstream os;
        auto uid = sanitize_id(c.userid);
        auto gid = sanitize_id(c.group);
        os << "apiVersion: rbac.authorization.k8s.io/v1\n";
        os << "kind: RoleBinding\n";
        os << "metadata:\n";
        os << "  name: " << uid << "-to-" << gid << "\n";
        os << "roleRef:\n";
        os << "  apiGroup: rbac.authorization.k8s.io\n";
        os << "  kind: Role\n";
        os << "  name: racf-" << gid << "\n";
        os << "subjects:\n";
        os << "  - kind: ServiceAccount\n";
        os << "    name: " << uid << "\n";
        return os.str();
    }

    // Emit ClusterRole for SPECIAL users
    static std::string emit_cluster_admin(const UserProfile& u) {
        std::ostringstream os;
        auto id = sanitize_id(u.userid);
        os << "apiVersion: rbac.authorization.k8s.io/v1\n";
        os << "kind: ClusterRoleBinding\n";
        os << "metadata:\n";
        os << "  name: " << id << "-admin\n";
        os << "roleRef:\n";
        os << "  apiGroup: rbac.authorization.k8s.io\n";
        os << "  kind: ClusterRole\n";
        os << "  name: cluster-admin\n";
        os << "subjects:\n";
        os << "  - kind: ServiceAccount\n";
        os << "    name: " << id << "\n";
        os << "    namespace: default\n";
        return os.str();
    }

    // Emit a dataset-specific Role
    static std::string emit_dataset_role(const DatasetProfile& ds) {
        std::ostringstream os;
        auto id = sanitize_id(ds.dsname);
        auto verbs = access_to_k8s_verbs(ds.uacc);
        os << "apiVersion: rbac.authorization.k8s.io/v1\n";
        os << "kind: Role\n";
        os << "metadata:\n";
        os << "  name: racf-ds-" << id << "\n";
        os << "  labels:\n";
        os << "    racf-dataset: \"" << ds.dsname << "\"\n";
        if (ds.generic) {
            os << "    racf-generic: \"true\"\n";
        }
        os << "rules:\n";
        os << "  - apiGroups: [\"\"]\n";
        os << "    resources: [\"configmaps\"]\n";
        os << "    resourceNames: [\"" << id << "\"]\n";
        os << "    verbs: [";
        for (size_t i = 0; i < verbs.size(); ++i) {
            if (i > 0) os << ", ";
            os << "\"" << verbs[i] << "\"";
        }
        os << "]\n";
        return os.str();
    }

private:
    static void emit_service_accounts(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& u : db.users) {
            os << emit_service_account(u) << "---\n";
        }
    }

    static void emit_roles(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& g : db.groups) {
            os << emit_role(g, db) << "---\n";
        }
        for (auto& ds : db.datasets) {
            os << emit_dataset_role(ds) << "---\n";
        }
    }

    static void emit_role_bindings(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& c : db.connects) {
            os << emit_role_binding(c) << "---\n";
        }
    }

    static void emit_admin_roles(std::ostringstream& os, const ProfileDatabase& db) {
        for (auto& u : db.users) {
            if (u.special) {
                os << emit_cluster_admin(u) << "---\n";
            }
        }
    }
};

}} // namespace lazarus::racf

#endif // LAZARUS_RACF_POLICY_EMITTER_H
