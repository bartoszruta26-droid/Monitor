#include "monitor/xetra_calendar_source.hpp"
#include "monitor/xetra_calendar_importer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace monitor::market {
namespace {
std::atomic<std::uint64_t> archive_counter{0};

bool allowed_xetra_url(const std::string& url) {
    constexpr std::string_view prefix="https://";
    if (url.rfind(prefix,0)!=0) return false;
    const auto end=url.find_first_of("/?#",prefix.size());
    const auto host=url.substr(prefix.size(),end==std::string::npos
        ? std::string::npos : end-prefix.size());
    return (host=="www.cashmarket.deutsche-boerse.com" ||
            host=="www.eurexgroup.com") &&
           url.find('@')==std::string::npos &&
           url.find('#')==std::string::npos;
}
std::string lower(std::string value) {
    std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
bool same_notice_payload(const VenueSessionNotice& a,const VenueSessionNotice& b) {
    if(a.notice_id!=b.notice_id || a.revision!=b.revision || a.calendar_id!=b.calendar_id ||
       a.venue!=b.venue || format_date(a.effective_date)!=format_date(b.effective_date) ||
       a.source_object_id!=b.source_object_id || a.source_uri!=b.source_uri ||
       a.attribution!=b.attribution || a.license_status!=b.license_status || a.kind!=b.kind ||
       a.day_status!=b.day_status || a.cancels_notice_id!=b.cancels_notice_id ||
       a.cancels_revision!=b.cancels_revision || a.product_sessions.size()!=b.product_sessions.size() ||
       a.phase_events.size()!=b.phase_events.size())
        return false;
    for(std::size_t i=0;i<a.product_sessions.size();++i) {
        const auto& x=a.product_sessions[i];
        const auto& y=b.product_sessions[i];
        if(x.product_group!=y.product_group ||
           x.opens_at.hour!=y.opens_at.hour || x.opens_at.minute!=y.opens_at.minute ||
           x.opens_at.second!=y.opens_at.second || x.closes_at.hour!=y.closes_at.hour ||
           x.closes_at.minute!=y.closes_at.minute || x.closes_at.second!=y.closes_at.second)
            return false;
    }
    for(std::size_t i=0;i<a.phase_events.size();++i) {
        const auto& x=a.phase_events[i];
        const auto& y=b.phase_events[i];
        if(x.kind!=y.kind || x.at.hour!=y.at.hour || x.at.minute!=y.at.minute ||
           x.at.second!=y.at.second || x.scope!=y.scope)
            return false;
    }
    return true;
}
std::string extract_pdf_text(std::string_view pdf_bytes) {
    if(pdf_bytes.empty() || pdf_bytes.size()>16U*1024U*1024U)
        throw std::invalid_argument("Plik PDF Xetra jest pusty lub przekracza limit 16 MiB.");
#ifndef MONITOR_PDFTOTEXT_EXECUTABLE
    throw std::runtime_error("Obsługa PDF wymaga zainstalowanego programu pdftotext.");
#else
    char temporary_path[]="/tmp/monitor-xetra-pdf-XXXXXX";
    const int file_fd=::mkstemp(temporary_path);
    if(file_fd<0) throw std::runtime_error("Nie można utworzyć tymczasowego pliku PDF.");
    int write_fd=file_fd;
    try {
        if(::fcntl(write_fd,F_SETFD,FD_CLOEXEC)!=0)
            throw std::runtime_error("Nie można ustawić bezpiecznych flag pliku PDF.");
        std::size_t offset=0;
        while(offset<pdf_bytes.size()) {
            const auto count=::write(write_fd,pdf_bytes.data()+offset,pdf_bytes.size()-offset);
            if(count<0) { if(errno==EINTR) continue; throw std::runtime_error("Nie można zapisać tymczasowego PDF."); }
            if(count==0) throw std::runtime_error("Zapis tymczasowego PDF nie postępuje.");
            offset+=static_cast<std::size_t>(count);
        }
        if(::fsync(write_fd)!=0) throw std::runtime_error("Nie można zsynchronizować tymczasowego PDF.");
        if(::close(write_fd)!=0) { write_fd=-1; throw std::runtime_error("Nie można zamknąć tymczasowego PDF."); }
        write_fd=-1;
    } catch(...) {
        if(write_fd>=0) ::close(write_fd);
        ::unlink(temporary_path);
        throw;
    }

    int pipe_fds[2]{};
    if(::pipe(pipe_fds)!=0) {
        ::unlink(temporary_path);
        throw std::runtime_error("Nie można utworzyć potoku dla pdftotext.");
    }
    const pid_t child=::fork();
    if(child<0) {
        ::close(pipe_fds[0]); ::close(pipe_fds[1]); ::unlink(temporary_path);
        throw std::runtime_error("Nie można uruchomić procesu pdftotext.");
    }
    if(child==0) {
        (void)::dup2(pipe_fds[1],STDOUT_FILENO);
        ::close(pipe_fds[0]); ::close(pipe_fds[1]);
        ::execl(MONITOR_PDFTOTEXT_EXECUTABLE,"pdftotext","-layout",temporary_path,"-",
                static_cast<char*>(nullptr));
        ::_exit(127);
    }
    ::close(pipe_fds[1]);
    std::string extracted;
    std::array<char,8192> buffer{};
    bool too_large=false;
    while(true) {
        const auto count=::read(pipe_fds[0],buffer.data(),buffer.size());
        if(count==0) break;
        if(count<0) {
            if(errno==EINTR) continue;
            ::kill(child,SIGKILL);
            ::close(pipe_fds[0]);
            int status=0; (void)::waitpid(child,&status,0);
            ::unlink(temporary_path);
            throw std::runtime_error("Błąd odczytu tekstu z PDF Xetra.");
        }
        const auto size=static_cast<std::size_t>(count);
        if(extracted.size()>2U*1024U*1024U || size>2U*1024U*1024U-extracted.size()) {
            too_large=true;
            ::kill(child,SIGKILL);
            break;
        }
        extracted.append(buffer.data(),size);
    }
    ::close(pipe_fds[0]);
    int status=0;
    while(::waitpid(child,&status,0)<0) {
        if(errno==EINTR) continue;
        ::unlink(temporary_path);
        throw std::runtime_error("Nie można odebrać wyniku procesu pdftotext.");
    }
    ::unlink(temporary_path);
    if(too_large) throw std::runtime_error("Tekst wydobyty z PDF przekracza limit 2 MiB.");
    if(!WIFEXITED(status) || WEXITSTATUS(status)!=0)
        throw std::runtime_error("pdftotext nie rozpoznał oficjalnego PDF Xetra.");
    if(extracted.empty()) throw std::runtime_error("PDF Xetra nie zawiera wydobywalnego tekstu.");
    return extracted;
#endif
}
struct DownloadedCalendar {
    VenueSessionCalendar calendar;
    std::string raw_html;
    std::string content_type;
};
DownloadedCalendar download(HttpTransport& transport,SourceCalendarDocument source) {
    validate_source_calendar_document(source);
    if (!allowed_xetra_url(source.source_uri))
        throw std::invalid_argument("Adres kalendarza Xetra musi używać zatwierdzonego hosta HTTPS.");
    if (source.retrieved_at || source.content_sha256 || source.source_object_id ||
        source.archive_relative_path || source.content_length_bytes || source.content_type)
        throw std::invalid_argument("Metadane pobrania, obiektu i archiwum ustala importer, nie wywołujący.");
    HttpRequest request;
    request.url=source.source_uri;
    request.timeout=std::chrono::milliseconds{15000};
    request.max_response_bytes=4U*1024U*1024U;
    request.headers={{"Accept","text/html"},{"User-Agent","Monitor-Rynkow/0.2 C++20"}};
    const auto response=transport.get(request);
    if (response.status_code!=200)
        throw std::runtime_error("Oficjalne źródło Xetra zwróciło HTTP "+std::to_string(response.status_code)+".");
    if (response.body.empty() || response.body.size()>request.max_response_bytes)
        throw std::runtime_error("Odpowiedź HTML Xetra jest pusta lub przekracza limit.");
    const auto it=response.headers.find("content-type");
    if (it==response.headers.end())
        throw std::runtime_error("Odpowiedź HTML Xetra nie ma nagłówka Content-Type.");
    const auto type=lower(it->second);
    if (type!="text/html" && type.rfind("text/html;",0)!=0)
        throw std::runtime_error("Źródło Xetra zwróciło nieoczekiwany typ treści.");
    source.content_sha256=sha256_hex(response.body);
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    source.retrieved_at=UtcTimestamp{now};
    auto calendar=parse_xetra_html_calendar(response.body,source);
    return DownloadedCalendar{std::move(calendar),response.body,it->second};
}
void archive_exact_bytes(const std::filesystem::path& path,const std::string& bytes) {
    std::filesystem::create_directories(path.parent_path());
    auto temporary=path;
    temporary += ".tmp."+std::to_string(::getpid())+"."+std::to_string(archive_counter.fetch_add(1));
    const int fd=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd<0) throw std::runtime_error("Nie można utworzyć tymczasowego archiwum HTML: "+std::string(std::strerror(errno)));
    bool open_fd=true;
    try {
        std::size_t offset=0;
        while(offset<bytes.size()) {
            const auto written=::write(fd,bytes.data()+offset,bytes.size()-offset);
            if(written<0) {
                if(errno==EINTR) continue;
                throw std::runtime_error("Błąd zapisu tymczasowego archiwum HTML: "+std::string(std::strerror(errno)));
            }
            if(written==0) throw std::runtime_error("Zapis archiwum HTML nie postępuje.");
            offset+=static_cast<std::size_t>(written);
        }
        if(::fsync(fd)!=0)
            throw std::runtime_error("Nie można zsynchronizować archiwum HTML: "+std::string(std::strerror(errno)));
        if(::close(fd)!=0) {
            open_fd=false;
            throw std::runtime_error("Nie można zamknąć tymczasowego archiwum HTML.");
        }
        open_fd=false;

        // link() publishes without replacing an existing immutable object.
        if(::link(temporary.c_str(),path.c_str())!=0) {
            const int link_error=errno;
            if(link_error!=EEXIST)
                throw std::runtime_error("Nie udało się opublikować archiwum HTML: "+std::string(std::strerror(link_error)));
            const int existing_fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
            if(existing_fd<0)
                throw std::runtime_error("Istniejące archiwum jest niedostępne lub jest dowiązaniem symbolicznym.");
            struct stat st{};
            if(::fstat(existing_fd,&st)!=0 || !S_ISREG(st.st_mode)) {
                ::close(existing_fd);
                throw std::runtime_error("Istniejące archiwum nie jest zwykłym plikiem.");
            }
            std::string stored;
            std::array<char,65536> buffer{};
            while(true) {
                const auto count=::read(existing_fd,buffer.data(),buffer.size());
                if(count==0) break;
                if(count<0) {
                    if(errno==EINTR) continue;
                    ::close(existing_fd);
                    throw std::runtime_error("Błąd odczytu istniejącego archiwum HTML.");
                }
                stored.append(buffer.data(),static_cast<std::size_t>(count));
            }
            ::close(existing_fd);
            if(stored!=bytes)
                throw std::runtime_error("Konflikt: archiwum SHA-256 istnieje, ale bajty są różne.");
        } else {
            const int directory_fd=::open(path.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
            if(directory_fd<0)
                throw std::runtime_error("Nie można otworzyć katalogu archiwum HTML do synchronizacji.");
            const int sync_result=::fsync(directory_fd);
            const int sync_error=errno;
            ::close(directory_fd);
            if(sync_result!=0)
                throw std::runtime_error("Nie można zsynchronizować katalogu archiwum HTML: "+std::string(std::strerror(sync_error)));
        }
        if(::unlink(temporary.c_str())!=0)
            throw std::runtime_error("Nie można usunąć tymczasowego pliku archiwum HTML.");
    } catch(...) {
        if(open_fd) ::close(fd);
        ::unlink(temporary.c_str());
        throw;
    }
}
}

bool xetra_pdf_import_available() noexcept {
#ifdef MONITOR_PDFTOTEXT_EXECUTABLE
    return true;
#else
    return false;
#endif
}
VenueSessionCalendar XetraCalendarSourceAdapter::fetch(SourceCalendarDocument source) const {
    return download(transport_,std::move(source)).calendar;
}
namespace {
std::uint64_t import_xetra_calendar(
    HttpTransport& transport,JsonMarketDataRepository& repository,
    SourceCalendarDocument source,std::string calendar_id,
    std::uint32_t calendar_revision,bool deduplicate_by_source_document) {
    if(calendar_id.empty() || calendar_revision==0U)
        throw std::invalid_argument("Import kalendarza wymaga identyfikatora i dodatniej rewizji.");
    auto downloaded=download(transport,std::move(source));
    auto& metadata=downloaded.calendar.source;
    const auto hash=*metadata.content_sha256;
    downloaded.calendar.calendar_id=std::move(calendar_id);
    downloaded.calendar.revision=calendar_revision;
    const auto archive_root=repository.file_path().parent_path()/
        (repository.file_path().filename().string()+".sources")/"objects";
    const auto archive_path=archive_root/(hash+".html");
    // Persist exact source bytes before publishing any calendar reference.
    archive_exact_bytes(archive_path,downloaded.raw_html);
    const auto relative_archive=std::filesystem::relative(archive_path,
        repository.file_path().parent_path().empty() ? std::filesystem::path(".")
                                                     : repository.file_path().parent_path());

    SourceArchiveObject source_object;
    source_object.source_object_id="sha256:"+hash;
    source_object.content_sha256=hash;
    source_object.archive_relative_path=relative_archive.generic_string();
    source_object.content_length_bytes=static_cast<std::uint64_t>(downloaded.raw_html.size());
    source_object.content_type=downloaded.content_type;
    metadata.source_object_id=source_object.source_object_id;
    metadata.archive_relative_path.reset();
    metadata.content_length_bytes.reset();
    metadata.content_type.reset();

    auto snapshot=repository.load();
    const auto duplicate=std::find_if(snapshot.venue_calendars.begin(),
        snapshot.venue_calendars.end(),[&](const VenueSessionCalendar& item) {
            if(deduplicate_by_source_document)
                return item.source.document_id==metadata.document_id &&
                       item.source.content_sha256==metadata.content_sha256;
            return item.calendar_id==downloaded.calendar.calendar_id &&
                   item.revision==downloaded.calendar.revision;
        });
    if(duplicate!=snapshot.venue_calendars.end()) {
        if(duplicate->source.content_sha256!=metadata.content_sha256)
            throw std::invalid_argument("Opublikowana rewizja kalendarza wskazuje już inne bajty źródłowe.");
        const auto existing_object=std::find_if(snapshot.source_objects.begin(),
            snapshot.source_objects.end(),[&](const SourceArchiveObject& item) {
                return item.source_object_id==source_object.source_object_id;
            });
        if(existing_object==snapshot.source_objects.end())
            (void)repository.upsert_source_object(source_object);
        return snapshot.revision;
    }
    MarketDataBatch batch;
    batch.source_objects.push_back(std::move(source_object));
    batch.venue_calendars.push_back(std::move(downloaded.calendar));
    return repository.upsert_batch(batch);
}
}
namespace {
std::string read_source_object_bytes(const JsonMarketDataRepository& repository,
                                     const SourceArchiveObject& object) {
    const std::filesystem::path relative(object.archive_relative_path);
    if(relative.empty() || relative.is_absolute() || relative.has_root_name() ||
       relative.has_root_directory() || object.content_length_bytes>4U*1024U*1024U)
        throw std::invalid_argument("Ścieżka lub rozmiar archiwum źródłowego jest nieprawidłowy.");
    for(const auto& component:relative)
        if(component.empty() || component=="." || component=="..")
            throw std::invalid_argument("Ścieżka archiwum źródłowego zawiera niedozwolony komponent.");
    auto root=repository.file_path().parent_path();
    if(root.empty()) root=".";
    int current_fd=::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(current_fd<0) throw std::runtime_error("Nie można bezpiecznie otworzyć katalogu repozytorium.");
    for(auto it=relative.begin();it!=relative.end();++it) {
        auto next=it;
        ++next;
        const bool final_component=next==relative.end();
        const int flags=O_RDONLY|O_CLOEXEC|O_NOFOLLOW|(final_component?0:O_DIRECTORY);
        const auto name=it->string();
        const int next_fd=::openat(current_fd,name.c_str(),flags);
        const int open_error=errno;
        ::close(current_fd);
        if(next_fd<0) {
            errno=open_error;
            throw std::runtime_error("Nie można bezpiecznie otworzyć komponentu archiwum źródłowego.");
        }
        current_fd=next_fd;
    }
    struct stat st{};
    if(::fstat(current_fd,&st)!=0 || !S_ISREG(st.st_mode) || st.st_size<0 ||
       static_cast<std::uint64_t>(st.st_size)!=object.content_length_bytes) {
        ::close(current_fd);
        throw std::runtime_error("Plik archiwum źródłowego ma nieprawidłowy typ lub długość.");
    }
    std::string bytes;
    bytes.reserve(static_cast<std::size_t>(st.st_size));
    std::array<char,65536> buffer{};
    while(true) {
        const auto count=::read(current_fd,buffer.data(),buffer.size());
        if(count==0) break;
        if(count<0) {
            if(errno==EINTR) continue;
            ::close(current_fd);
            throw std::runtime_error("Błąd odczytu archiwum źródłowego.");
        }
        bytes.append(buffer.data(),static_cast<std::size_t>(count));
    }
    ::close(current_fd);
    if(sha256_hex(bytes)!=object.content_sha256)
        throw std::runtime_error("SHA-256 archiwum źródłowego nie zgadza się z obiektem.");
    return bytes;
}
}
std::uint64_t XetraCalendarSourceAdapter::import(
    JsonMarketDataRepository& repository,SourceCalendarDocument source) const {
    const auto legacy_calendar_id=source.document_id;
    const auto legacy_revision=source.version;
    return import_xetra_calendar(transport_,repository,std::move(source),
                                 legacy_calendar_id,legacy_revision,true);
}
std::uint64_t XetraCalendarSourceAdapter::import(
    JsonMarketDataRepository& repository,SourceCalendarDocument source,
    std::string calendar_id,std::uint32_t calendar_revision) const {
    return import_xetra_calendar(transport_,repository,std::move(source),
                                 std::move(calendar_id),calendar_revision,false);
}
std::uint64_t XetraCalendarSourceAdapter::import_notice(
    JsonMarketDataRepository& repository,VenueSessionNotice notice,std::string source_uri) const {
    if(!allowed_xetra_url(source_uri))
        throw std::invalid_argument("Adres komunikatu Xetra musi używać zatwierdzonego hosta HTTPS.");
    if(notice.source_object_id.size()!=0U || notice.retrieved_at || !notice.source_uri.empty())
        throw std::invalid_argument("Metadane pobrania komunikatu ustala importer, nie wywołujący.");
    HttpRequest request;
    request.url=source_uri;
    request.timeout=std::chrono::milliseconds{15000};
    request.max_response_bytes=4U*1024U*1024U;
    request.headers={{"Accept","text/html"},{"User-Agent","Monitor-Rynkow/0.2 C++20"}};
    const auto response=transport_.get(request);
    if(response.status_code!=200)
        throw std::runtime_error("Oficjalne źródło komunikatu Xetra zwróciło HTTP "+std::to_string(response.status_code)+".");
    if(response.body.empty() || response.body.size()>request.max_response_bytes)
        throw std::runtime_error("Odpowiedź komunikatu Xetra jest pusta lub przekracza limit.");
    const auto type_it=response.headers.find("content-type");
    if(type_it==response.headers.end())
        throw std::runtime_error("Odpowiedź komunikatu Xetra nie ma nagłówka Content-Type.");
    const auto content_type=lower(type_it->second);
    if(content_type!="text/html" && content_type.rfind("text/html;",0)!=0)
        throw std::runtime_error("Źródło komunikatu Xetra zwróciło nieoczekiwany typ treści.");

    const auto hash=sha256_hex(response.body);
    notice.source_object_id="sha256:"+hash;
    notice.source_uri=source_uri;
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    notice.retrieved_at=UtcTimestamp{now};
    auto parsed_notice=parse_xetra_session_notice_html(response.body,std::move(notice));
    const auto existing_snapshot=repository.load();
    const auto existing_notice=std::find_if(existing_snapshot.venue_session_notices.begin(),
        existing_snapshot.venue_session_notices.end(),[&](const VenueSessionNotice& item) {
            return item.notice_id==parsed_notice.notice_id && item.revision==parsed_notice.revision;
        });
    if(existing_notice!=existing_snapshot.venue_session_notices.end()) {
        if(!same_notice_payload(*existing_notice,parsed_notice))
            throw std::invalid_argument("Opublikowana rewizja komunikatu Xetra wskazuje inną treść lub interpretację.");
        const auto existing_object=std::find_if(existing_snapshot.source_objects.begin(),
            existing_snapshot.source_objects.end(),[&](const SourceArchiveObject& item) {
                return item.source_object_id==parsed_notice.source_object_id;
            });
        if(existing_object==existing_snapshot.source_objects.end() ||
           existing_object->content_length_bytes!=static_cast<std::uint64_t>(response.body.size()) ||
           existing_object->content_type!=type_it->second)
            throw std::invalid_argument("Metadane obiektu źródłowego komunikatu nie zgadzają się z ponownym pobraniem.");
        return existing_snapshot.revision;
    }

    const auto archive_root=repository.file_path().parent_path()/
        (repository.file_path().filename().string()+".sources")/"objects";
    const auto archive_path=archive_root/(hash+".html");
    archive_exact_bytes(archive_path,response.body);
    const auto relative_archive=std::filesystem::relative(archive_path,
        repository.file_path().parent_path().empty() ? std::filesystem::path(".")
                                                     : repository.file_path().parent_path());
    SourceArchiveObject source_object;
    source_object.source_object_id="sha256:"+hash;
    source_object.content_sha256=hash;
    source_object.archive_relative_path=relative_archive.generic_string();
    source_object.content_length_bytes=static_cast<std::uint64_t>(response.body.size());
    source_object.content_type=type_it->second;

    MarketDataBatch batch;
    batch.source_objects.push_back(std::move(source_object));
    batch.venue_session_notices.push_back(std::move(parsed_notice));
    return repository.upsert_batch(batch);
}
std::uint64_t XetraCalendarSourceAdapter::import_pdf_notice(
    JsonMarketDataRepository& repository,VenueSessionNotice notice,std::string source_uri) const {
    if(!allowed_xetra_url(source_uri))
        throw std::invalid_argument("Adres PDF komunikatu Xetra musi używać zatwierdzonego hosta HTTPS.");
    if(!notice.source_object_id.empty() || notice.retrieved_at || !notice.source_uri.empty())
        throw std::invalid_argument("Metadane pobrania PDF ustala importer, nie wywołujący.");
    HttpRequest request;
    request.url=source_uri;
    request.timeout=std::chrono::milliseconds{20000};
    request.max_response_bytes=16U*1024U*1024U;
    request.headers={{"Accept","application/pdf"},{"User-Agent","Monitor-Rynkow/0.2 C++20"}};
    const auto response=transport_.get(request);
    if(response.status_code!=200)
        throw std::runtime_error("Oficjalne źródło PDF Xetra zwróciło HTTP "+std::to_string(response.status_code)+".");
    if(response.body.empty() || response.body.size()>request.max_response_bytes)
        throw std::runtime_error("Odpowiedź PDF Xetra jest pusta lub przekracza limit.");
    const auto type_it=response.headers.find("content-type");
    if(type_it==response.headers.end())
        throw std::runtime_error("Odpowiedź PDF Xetra nie ma nagłówka Content-Type.");
    const auto content_type=lower(type_it->second);
    if(content_type!="application/pdf" && content_type.rfind("application/pdf;",0)!=0)
        throw std::runtime_error("Źródło komunikatu Xetra nie zwróciło application/pdf.");

    const auto hash=sha256_hex(response.body);
    notice.source_object_id="sha256:"+hash;
    notice.source_uri=source_uri;
    const auto now=std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    notice.retrieved_at=UtcTimestamp{now};
    const auto extracted=extract_pdf_text(response.body);
    auto parsed_notice=parse_xetra_session_notice_text(extracted,std::move(notice));

    const auto existing_snapshot=repository.load();
    const auto existing_notice=std::find_if(existing_snapshot.venue_session_notices.begin(),
        existing_snapshot.venue_session_notices.end(),[&](const VenueSessionNotice& item) {
            return item.notice_id==parsed_notice.notice_id && item.revision==parsed_notice.revision;
        });
    if(existing_notice!=existing_snapshot.venue_session_notices.end()) {
        if(!same_notice_payload(*existing_notice,parsed_notice))
            throw std::invalid_argument("Opublikowana rewizja PDF komunikatu Xetra wskazuje inną treść lub interpretację.");
        const auto existing_object=std::find_if(existing_snapshot.source_objects.begin(),
            existing_snapshot.source_objects.end(),[&](const SourceArchiveObject& item) {
                return item.source_object_id==parsed_notice.source_object_id;
            });
        if(existing_object==existing_snapshot.source_objects.end() ||
           existing_object->content_length_bytes!=static_cast<std::uint64_t>(response.body.size()) ||
           existing_object->content_type!=type_it->second)
            throw std::invalid_argument("Metadane obiektu PDF nie zgadzają się z ponownym pobraniem.");
        return existing_snapshot.revision;
    }

    const auto archive_root=repository.file_path().parent_path()/
        (repository.file_path().filename().string()+".sources")/"objects";
    const auto archive_path=archive_root/(hash+".pdf");
    archive_exact_bytes(archive_path,response.body);
    const auto relative_archive=std::filesystem::relative(archive_path,
        repository.file_path().parent_path().empty() ? std::filesystem::path(".")
                                                     : repository.file_path().parent_path());
    SourceArchiveObject source_object;
    source_object.source_object_id="sha256:"+hash;
    source_object.content_sha256=hash;
    source_object.archive_relative_path=relative_archive.generic_string();
    source_object.content_length_bytes=static_cast<std::uint64_t>(response.body.size());
    source_object.content_type=type_it->second;

    MarketDataBatch batch;
    batch.source_objects.push_back(std::move(source_object));
    batch.venue_session_notices.push_back(std::move(parsed_notice));
    return repository.upsert_batch(batch);
}
std::uint64_t XetraCalendarSourceAdapter::publish_revision_from_archive(
    JsonMarketDataRepository& repository,SourceCalendarDocument source,
    std::string calendar_id,std::uint32_t calendar_revision,
    std::string source_object_id) const {
    if(calendar_id.empty() || calendar_revision==0U || source_object_id.empty())
        throw std::invalid_argument("Publikacja rewizji wymaga identyfikatora kalendarza, rewizji i obiektu źródłowego.");
    const auto snapshot=repository.load();
    const auto object=std::find_if(snapshot.source_objects.begin(),snapshot.source_objects.end(),
        [&](const SourceArchiveObject& item){return item.source_object_id==source_object_id;});
    if(object==snapshot.source_objects.end())
        throw std::invalid_argument("Nie znaleziono obiektu źródłowego do ponownej interpretacji.");
    if((source.content_sha256 && *source.content_sha256!=object->content_sha256) ||
       (source.source_object_id && *source.source_object_id!=source_object_id))
        throw std::invalid_argument("Metadane źródła nie odpowiadają wskazanemu obiektowi.");
    const auto existing=std::find_if(snapshot.venue_calendars.begin(),snapshot.venue_calendars.end(),
        [&](const VenueSessionCalendar& item) {
            return item.calendar_id==calendar_id && item.revision==calendar_revision;
        });
    if(existing!=snapshot.venue_calendars.end()) {
        if(existing->source.source_object_id!=source_object_id)
            throw std::invalid_argument("Rewizja kalendarza jest już opublikowana dla innego obiektu źródłowego.");
        return snapshot.revision;
    }
    if(!source.retrieved_at) {
        const auto provenance=std::find_if(snapshot.venue_calendars.begin(),snapshot.venue_calendars.end(),
            [&](const VenueSessionCalendar& item) {
                return item.source.source_object_id==source_object_id;
            });
        if(provenance!=snapshot.venue_calendars.end())
            source.retrieved_at=provenance->source.retrieved_at;
    }
    source.content_sha256=object->content_sha256;
    source.source_object_id=source_object_id;
    source.archive_relative_path.reset();
    source.content_length_bytes.reset();
    source.content_type.reset();
    const auto bytes=read_source_object_bytes(repository,*object);
    auto calendar=parse_xetra_html_calendar(bytes,source);
    calendar.calendar_id=std::move(calendar_id);
    calendar.revision=calendar_revision;
    return repository.upsert_venue_calendar(calendar);
}
} // namespace monitor::market
