#include "ldn_icommunication.hpp"

namespace ams::mitm::ldn {
    static_assert(sizeof(NetworkInfo) == 0x480, "sizeof(NetworkInfo) should be 0x480");
    static_assert(sizeof(ConnectNetworkData) == 0x7C, "sizeof(ConnectNetworkData) should be 0x7C");
    static_assert(sizeof(ScanFilter) == 0x60, "sizeof(ScanFilter) should be 0x60");

    // https://reswitched.github.io/SwIPC/ifaces.html#nn::ldn::detail::IUserLocalCommunicationService

    Result ICommunicationInterface::Initialize(const sf::ClientProcessId &client_process_id) {
        LogFormat("ICommunicationInterface::Initialize pid: %" PRIu64, client_process_id);

        if (this->state_event == nullptr) {
            this->state_event = new os::SystemEvent(true);
        }

        R_TRY(lanDiscovery.initialize([&](){
            this->onEventFired();
        }));

        return ResultSuccess();
    }

    Result ICommunicationInterface::InitializeSystem2(u64 unk, const sf::ClientProcessId &client_process_id) {
        LogFormat("ICommunicationInterface::InitializeSystem2 unk: %" PRIu64, unk);
        return this->Initialize(client_process_id);
    }

    Result ICommunicationInterface::Finalize() {
        Result rc = lanDiscovery.finalize();
        if (this->state_event) {
            this->state_event->Finalize();
            delete this->state_event;
            this->state_event = nullptr;
        }
        return rc;
    }

    Result ICommunicationInterface::OpenAccessPoint() {
        return this->lanDiscovery.openAccessPoint();
    }

    Result ICommunicationInterface::CloseAccessPoint() {
        return this->lanDiscovery.closeAccessPoint();
    }

    Result ICommunicationInterface::DestroyNetwork() {
        return this->lanDiscovery.destroyNetwork();
    }

    Result ICommunicationInterface::OpenStation() {
        return this->lanDiscovery.openStation();
    }

    Result ICommunicationInterface::CloseStation() {
        return this->lanDiscovery.closeStation();
    }

    Result ICommunicationInterface::Disconnect() {
        return this->lanDiscovery.disconnect();
    }

    Result ICommunicationInterface::CreateNetwork(CreateNetworkConfig data) {
        return this->lanDiscovery.createNetwork(&data.securityConfig, &data.userConfig, &data.networkConfig);;
    }

    Result ICommunicationInterface::SetAdvertiseData(sf::InAutoSelectBuffer data) {
        return lanDiscovery.setAdvertiseData(data.GetPointer(), data.GetSize());
    }

    Result ICommunicationInterface::SetStationAcceptPolicy(u8 policy) {
        return 0;
    }

    Result ICommunicationInterface::SetWirelessControllerRestriction() {
        return 0;
    }

    Result ICommunicationInterface::GetState(sf::Out<u32> state) {
        state.SetValue(static_cast<u32>(this->lanDiscovery.getState()));

        return 0;
    }

    Result ICommunicationInterface::GetIpv4Address(sf::Out<u32> address, sf::Out<u32> netmask) {
        Result rc = ipinfoGetIpConfig(address.GetPointer(), netmask.GetPointer());

        LogFormat("get_ipv4_address %x %x", address.GetValue(), netmask.GetValue());

        return rc;
    }

    Result ICommunicationInterface::GetNetworkInfo(sf::Out<NetworkInfo> buffer) {
        LogFormat("get_network_info %p state: %d", buffer.GetPointer(), static_cast<u32>(this->lanDiscovery.getState()));

        return lanDiscovery.getNetworkInfo(buffer.GetPointer());
    }

    Result ICommunicationInterface::GetDisconnectReason(sf::Out<u32> reason) {
        reason.SetValue(0);

        return 0;
    }

    Result ICommunicationInterface::GetNetworkInfoLatestUpdate(sf::Out<NetworkInfo> buffer, sf::OutArray<NodeLatestUpdate> pUpdates) {
        LogFormat("get_network_info_latest buffer %p", buffer.GetPointer());
        LogFormat("get_network_info_latest pUpdates %p %" PRIu64, pUpdates.GetPointer(), pUpdates.GetSize());

        return lanDiscovery.getNetworkInfo(buffer.GetPointer(), pUpdates.GetPointer(), pUpdates.GetSize());
    }

    Result ICommunicationInterface::GetSecurityParameter(sf::Out<SecurityParameter> out) {
        Result rc = 0;

        SecurityParameter data;
        NetworkInfo info;
        rc = lanDiscovery.getNetworkInfo(&info);
        if (R_SUCCEEDED(rc)) {
            NetworkInfo2SecurityParameter(&info, &data);
            out.SetValue(data);
        }

        return rc;
    }

    Result ICommunicationInterface::GetNetworkConfig(sf::Out<NetworkConfig> out) {
        Result rc = 0;

        NetworkConfig data;
        NetworkInfo info;
        rc = lanDiscovery.getNetworkInfo(&info);
        if (R_SUCCEEDED(rc)) {
            NetworkInfo2NetworkConfig(&info, &data);
            out.SetValue(data);
        }

        return rc;
    }

    Result ICommunicationInterface::AttachStateChangeEvent(sf::Out<sf::CopyHandle> handle) {
        handle.SetValue(this->state_event->GetReadableHandle());
        return ResultSuccess();
    }

    Result ICommunicationInterface::Scan(sf::Out<u32> outCount, sf::OutAutoSelectArray<NetworkInfo> buffer, u16 channel, ScanFilter filter) {
        Result rc = 0;
        u16 count = buffer.GetSize();

        rc = lanDiscovery.scan(buffer.GetPointer(), &count, filter);
        outCount.SetValue(count);

        LogFormat("scan %d %d", count, rc);

        return rc;
    }

    Result ICommunicationInterface::Connect(ConnectNetworkData param, NetworkInfo &data) {
        LogFormat("ICommunicationInterface::connect");
        LogHex(&data, sizeof(NetworkInfo));
        LogHex(&param, sizeof(param));

        return lanDiscovery.connect(&data, &param.userConfig, param.localCommunicationVersion);
    }

    void ICommunicationInterface::onEventFired() {
        if (this->state_event) {
            LogFormat("onEventFired signal_event");
            this->state_event->Signal();
        }
    }
}
