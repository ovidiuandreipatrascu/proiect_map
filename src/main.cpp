// Wires the HTTP routes of the common contract.
// Add the routes required by your assigned theme below.

#include <chrono>
#include <iostream>
#include <optional>
#include <string>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "app.hpp"

int main() {
    using namespace map_project;

    const auto started_at = std::chrono::steady_clock::now();
    Store store;
    httplib::Server server;

    server.Get("/health", [&](const httplib::Request&, httplib::Response& response) {
        response.set_content(HealthBody(started_at).dump(), "application/json; charset=utf-8");
    });

    server.Get("/version", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(VersionBody().dump(), "application/json; charset=utf-8");
    });

    server.Post("/reset", [&](const httplib::Request&, httplib::Response& response) {
        store.Reset();
        response.status = 204;
    });

    server.Get("/", [](const httplib::Request&, httplib::Response& response) {
        response.set_content(HomePage(), "text/html; charset=utf-8");
    });

    // Theme 1: Contact Agenda - POST /contacts
    server.Post("/contacts", [&](const httplib::Request& request, httplib::Response& response) {
        nlohmann::json body;
        try {
            body = nlohmann::json::parse(request.body);
        } catch (...) {
            response.status = 400;
            response.set_content(ErrorBody("validation", "invalid json body").dump(),
                                 "application/json; charset=utf-8");
            return;
        }

        if (!body.is_object() ||
            !body.contains("name") || !body["name"].is_string() ||
            !body.contains("email") || !body["email"].is_string() ||
            !body.contains("phone") || !body["phone"].is_string() ||
            !body.contains("category") || !body["category"].is_string()) {
            response.status = 400;
            response.set_content(ErrorBody("validation", "missing or invalid required fields").dump(),
                                 "application/json; charset=utf-8");
            return;
        }

        const std::string name = body["name"].get<std::string>();
        const std::string email = body["email"].get<std::string>();
        const std::string phone = body["phone"].get<std::string>();
        const std::string category = body["category"].get<std::string>();

        if (!IsValidName(name) || !IsValidEmail(email) || !IsValidPhone(phone) || !IsValidCategory(category)) {
            response.status = 400;
            response.set_content(ErrorBody("validation", "field validation failed").dump(),
                                 "application/json; charset=utf-8");
            return;
        }

        if (store.HasEmail(email)) {
            response.status = 409;
            response.set_content(ErrorBody("conflict", "email already exists").dump(),
                                 "application/json; charset=utf-8");
            return;
        }

        const auto contact = store.AddContact(name, email, phone, category);
        response.status = 201;
        response.set_content(ContactToJson(contact).dump(), "application/json; charset=utf-8");
    });

    // Theme 1: Contact Agenda - GET /contacts
    server.Get("/contacts", [&](const httplib::Request& request, httplib::Response& response) {
        std::optional<std::string> category = std::nullopt;
        if (request.has_param("category")) {
            category = request.get_param_value("category");
        }
        std::optional<std::string> q = std::nullopt;
        if (request.has_param("q")) {
            q = request.get_param_value("q");
        }

        const auto contacts = store.GetContacts(category, q);
        response.status = 200;
        response.set_content(ContactsToJson(contacts).dump(), "application/json; charset=utf-8");
    });

    // Theme 1: Contact Agenda - GET /contacts/{id}
    server.Get(R"(/contacts/(\d+))", [&](const httplib::Request& request, httplib::Response& response) {
        try {
            const int id = std::stoi(request.matches[1]);
            const auto contact = store.GetContact(id);
            if (!contact) {
                response.status = 404;
                response.set_content(ErrorBody("not_found", "contact not found").dump(),
                                     "application/json; charset=utf-8");
                return;
            }
            response.status = 200;
            response.set_content(ContactToJson(*contact).dump(), "application/json; charset=utf-8");
        } catch (...) {
            response.status = 404;
            response.set_content(ErrorBody("not_found", "contact not found").dump(),
                                 "application/json; charset=utf-8");
        }
    });

    server.set_error_handler([](const httplib::Request&, httplib::Response& response) {
        if (response.status == 404) {
            response.set_content(ErrorBody("not_found", "route does not exist").dump(),
                                 "application/json; charset=utf-8");
        }
    });

    std::cout << kAppName << " " << kAppVersion << " starting on port 8080" << std::endl;
    if (!server.listen("0.0.0.0", 8080)) {
        std::cerr << "failed to bind port 8080" << std::endl;
        return 1;
    }
    return 0;
}
