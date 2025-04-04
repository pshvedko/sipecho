package main

import (
	"github.com/google/uuid"
	"log"
	"strings"
	"time"

	"github.com/eclipse/paho.mqtt.golang"

	"google.golang.org/protobuf/proto"

	"github.com/pshvedko/sipecho/example/sip"
)

func main() {
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

	requests := map[uuid.UUID]uuid.UUID{}

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
				message.Response = sip.NewResponse(200)
			default:
				var id uuid.UUID
				id, message.Id = message.Id.GetUUID(), sip.NewUUID()
				requests[message.Id.GetUUID()] = id
			}
		} else {
			switch topic[1] {
			case "registration":
				continue
			default:
				id, ok := requests[message.Id.GetUUID()]
				if !ok {
					continue
				}
				message.Id = sip.NewUUIDFrom(id)
			}
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
