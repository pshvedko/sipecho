package main

import (
	"log"
	"strings"
	"time"

	"github.com/eclipse/paho.mqtt.golang"

	xxx "github.com/yosssi/gmq/mqtt"
	xxx3 "github.com/yosssi/gmq/mqtt/client"

	"google.golang.org/protobuf/proto"

	"github.com/pshvedko/sipecho/example/sip"
)

func main() {

	_ = xxx3.PublishOptions{
		QoS:       xxx.QoS0,
		Retain:    false,
		TopicName: nil,
		Message:   nil,
	}
	_ = xxx3.ConnectOptions{
		Network:         "",
		Address:         "",
		TLSConfig:       nil,
		CONNACKTimeout:  0,
		PINGRESPTimeout: 0,
		ClientID:        nil,
		UserName:        nil,
		Password:        nil,
		CleanSession:    false,
		KeepAlive:       0,
		WillTopic:       nil,
		WillMessage:     nil,
		WillQoS:         0,
		WillRetain:      false,
	}

	messages := make(chan mqtt.Message)

	opts := mqtt.NewClientOptions()
	opts.AddBroker("tcp://uru:1883")
	opts.SetClientID("route333")
	opts.SetKeepAlive(60 * time.Second)
	opts.SetOnConnectHandler(func(client mqtt.Client) {
		token := client.Subscribe("Route/#", 0, func(client mqtt.Client, message mqtt.Message) {
			messages <- message
		})
		token.Wait()
	})

	client := mqtt.NewClient(opts)
	token := client.Connect()
	token.Wait()

	for m := range messages {
		topic := strings.Split(m.Topic(), "/")

		var message sip.Message
		err := proto.Unmarshal(m.Payload(), &message)
		if err != nil {
			log.Fatalln(err)
		}

		log.Println(">>>", topic, &message)

		if message.IsRequest() {
			switch topic[1] {

			case "registration":
				// return response
				message.Response = sip.NewResponse(200)

			case "invite":
				// return message back

			default:
				continue
			}
		} else {
			// return message back
		}

		topic[0] = "Sip"

		log.Println("<<<", topic, &message)

		bytes, err := proto.Marshal(&message)
		if err != nil {
			log.Fatalln(err)
		}

		token = client.Publish(strings.Join(topic, "/"), 0, false, bytes)
		token.Wait()
	}
}
